#include<iostream>
#include<string>
#include<vector>
#include<fstream>
#include<sstream>
#include<algorithm>
#include<chrono>
#include<cstdint>

#define rep(i,n) for(int i=0;i<n;i++)
#define BOARD_SIZE 8
#define DEPTH 5
#define othello std::vector<std::vector<int>>
#define INF 99999
#define DEBUG_OPTION false
#define LEARNING_DEPTH 3

using namespace std;
using std::cout;

// ============================================================
// === Phase 3: ビットボード (BitBoard) ===
// ============================================================
//
// ビットボードとは：
//   8x8=64マスの盤面を、64ビット整数(uint64_t)を2つ使って表現する。
//   black: 先手●の石がある位置のビットが1
//   white: 後手○の石がある位置のビットが1
//
// ビットの位置:
//   マス(i,j) = ビット i*8+j
//   例: (0,0)=bit0, (0,7)=bit7, (7,7)=bit63
//
// なぜ速い？:
//   合法手生成や石の反転がビット演算(AND,OR,SHIFT)だけで完結する。
//   vector<vector<int>>版では64回のループが必要だった処理が、
//   数回のビット演算で終わるため 10～50倍 高速になる。

// --- 端のマスク（ビットシフト時に盤面の端で折り返らないようにする）---
// A列(j=0)のビットを除外: 左方向にシフトするときに使う
const uint64_t NOT_A_FILE = 0xFEFEFEFEFEFEFEFEULL;
// H列(j=7)のビットを除外: 右方向にシフトするときに使う
const uint64_t NOT_H_FILE = 0x7F7F7F7F7F7F7F7FULL;
// AB列を除外: 2ビット左シフト用
const uint64_t NOT_AB_FILE = 0xFCFCFCFCFCFCFCFCULL;
// GH列を除外: 2ビット右シフト用
const uint64_t NOT_GH_FILE = 0x3F3F3F3F3F3F3F3FULL;

struct BitBoard {
	uint64_t black; // 先手●の石 (値=1に対応)
	uint64_t white; // 後手○の石 (値=-1に対応)

	// --- 初期盤面の生成 ---
	static BitBoard initial() {
		BitBoard bb;
		bb.black = 0;
		bb.white = 0;
		// d4,e5 が先手●(black)  d5,e4 が後手○(white)
		// マス(3,3)=bit27, (4,4)=bit36, (3,4)=bit28, (4,3)=bit35
		bb.black = (1ULL << 27) | (1ULL << 36);
		bb.white = (1ULL << 28) | (1ULL << 35);
		return bb;
	}

	// --- 合法手生成 ---
	// 指定した側(is_black=trueなら先手●)の合法手をビットマスクで返す。
	// 各方向に「相手の石をたどって自分の石に到達するか」をビット演算で一括検索。
	// ループなしで8方向同時に検査できるため高速。
	uint64_t get_legal_moves(bool is_black) const {
		uint64_t my = is_black ? black : white;
		uint64_t opp = is_black ? white : black;
		uint64_t empty = ~(black | white);
		uint64_t legal = 0;

		// 8方向それぞれについて、
		// 相手の石をたどって挟める位置を見つける
		// shift方向: 上(>>8), 下(<<8), 左(>>1), 右(<<1),
		//            左上(>>9), 右上(>>7), 左下(<<7), 右下(<<9)

		// --- 左方向 (>>1) ---
		uint64_t t = opp & NOT_A_FILE;  // 相手の石（A列除外）
		uint64_t flip = t & (my >> 1);
		flip |= t & (flip >> 1);
		flip |= t & (flip >> 1);
		flip |= t & (flip >> 1);
		flip |= t & (flip >> 1);
		flip |= t & (flip >> 1);
		legal |= empty & (flip >> 1);

		// --- 右方向 (<<1) ---
		t = opp & NOT_H_FILE;
		flip = t & (my << 1);
		flip |= t & (flip << 1);
		flip |= t & (flip << 1);
		flip |= t & (flip << 1);
		flip |= t & (flip << 1);
		flip |= t & (flip << 1);
		legal |= empty & (flip << 1);

		// --- 上方向 (>>8) ---
		t = opp;
		flip = t & (my >> 8);
		flip |= t & (flip >> 8);
		flip |= t & (flip >> 8);
		flip |= t & (flip >> 8);
		flip |= t & (flip >> 8);
		flip |= t & (flip >> 8);
		legal |= empty & (flip >> 8);

		// --- 下方向 (<<8) ---
		flip = t & (my << 8);
		flip |= t & (flip << 8);
		flip |= t & (flip << 8);
		flip |= t & (flip << 8);
		flip |= t & (flip << 8);
		flip |= t & (flip << 8);
		legal |= empty & (flip << 8);

		// --- 左上 (>>9) ---
		t = opp & NOT_A_FILE;
		flip = t & (my >> 9);
		flip |= t & (flip >> 9);
		flip |= t & (flip >> 9);
		flip |= t & (flip >> 9);
		flip |= t & (flip >> 9);
		flip |= t & (flip >> 9);
		legal |= empty & (flip >> 9);

		// --- 右上 (>>7) ---
		t = opp & NOT_H_FILE;
		flip = t & (my >> 7);
		flip |= t & (flip >> 7);
		flip |= t & (flip >> 7);
		flip |= t & (flip >> 7);
		flip |= t & (flip >> 7);
		flip |= t & (flip >> 7);
		legal |= empty & (flip >> 7);

		// --- 左下 (<<7) ---
		t = opp & NOT_A_FILE;
		flip = t & (my << 7);
		flip |= t & (flip << 7);
		flip |= t & (flip << 7);
		flip |= t & (flip << 7);
		flip |= t & (flip << 7);
		flip |= t & (flip << 7);
		legal |= empty & (flip << 7);

		// --- 右下 (<<9) ---
		t = opp & NOT_H_FILE;
		flip = t & (my << 9);
		flip |= t & (flip << 9);
		flip |= t & (flip << 9);
		flip |= t & (flip << 9);
		flip |= t & (flip << 9);
		flip |= t & (flip << 9);
		legal |= empty & (flip << 9);

		return legal;
	}

	// --- 1方向の反転ビットを計算 ---
	// posに石を置いたとき、dir方向に挟める石のビットマスクを返す
	static uint64_t calc_flip_dir(uint64_t pos, uint64_t my, uint64_t opp,
	                              int shift, uint64_t mask) {
		uint64_t flipped = 0;
		uint64_t t = opp & mask;
		uint64_t cursor;
		if (shift > 0) {
			cursor = (pos << shift) & t;
			while (cursor) {
				flipped |= cursor;
				cursor = (cursor << shift) & t;
			}
			// 最後に自分の石で挟まれているか確認
			if (!(((flipped << shift) & ~t) & my)) flipped = 0;
		} else {
			int rshift = -shift;
			cursor = (pos >> rshift) & t;
			while (cursor) {
				flipped |= cursor;
				cursor = (cursor >> rshift) & t;
			}
			if (!(((flipped >> rshift) & ~t) & my)) flipped = 0;
		}
		return flipped;
	}

	// --- 石を置く（反転処理込み）---
	// pos: 置く位置のビット（1ビットだけが1）
	// is_black: true=先手●が置く
	BitBoard place(uint64_t pos, bool is_black) const {
		uint64_t my = is_black ? black : white;
		uint64_t opp = is_black ? white : black;

		// 8方向の反転ビットを合算
		uint64_t flipped = 0;
		flipped |= calc_flip_dir(pos, my, opp,  1, NOT_H_FILE);  // 右
		flipped |= calc_flip_dir(pos, my, opp, -1, NOT_A_FILE);  // 左
		flipped |= calc_flip_dir(pos, my, opp,  8, ~0ULL);       // 下
		flipped |= calc_flip_dir(pos, my, opp, -8, ~0ULL);       // 上
		flipped |= calc_flip_dir(pos, my, opp,  9, NOT_H_FILE);  // 右下
		flipped |= calc_flip_dir(pos, my, opp, -9, NOT_A_FILE);  // 左上
		flipped |= calc_flip_dir(pos, my, opp,  7, NOT_A_FILE);  // 左下
		flipped |= calc_flip_dir(pos, my, opp, -7, NOT_H_FILE);  // 右上

		// 石を反転：自分に加え、相手から除く
		my |= pos | flipped;
		opp &= ~flipped;

		BitBoard result;
		if (is_black) { result.black = my; result.white = opp; }
		else { result.black = opp; result.white = my; }
		return result;
	}

	// --- 石数をカウント (popcount) ---
	// ビットが1の数 = 石の数。コンパイラ組み込み命令で1クロック。
	int count_black() const { return __builtin_popcountll(black); }
	int count_white() const { return __builtin_popcountll(white); }

	// --- 勝敗判定 ---
	// 1=先手●勝ち, -1=後手○勝ち, 0=引き分け
	int judge() const {
		int b = count_black(), w = count_white();
		if (b > w) return 1;
		if (w > b) return -1;
		return 0;
	}

	// --- 評価関数（int weight版）---
	// 後手○(AI)が有利なほどスコアが高い
	int evaluate(const int w[BOARD_SIZE][BOARD_SIZE]) const {
		int score = 0;
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
				int bit = i * 8 + j;
				if (white & (1ULL << bit)) score += w[i][j];
				else if (black & (1ULL << bit)) score -= w[i][j];
			}
		}
		return score;
	}

	// --- 評価関数（double weight版）---
	// TD学習で使用。weight_dを使ってdouble精度で計算。
	double evaluate_d(const double wd[BOARD_SIZE][BOARD_SIZE]) const {
		double score = 0.0;
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
				int bit = i * 8 + j;
				if (white & (1ULL << bit)) score += wd[i][j];
				else if (black & (1ULL << bit)) score -= wd[i][j];
			}
		}
		return score;
	}

	// --- othello(vector<vector<int>>)への変換 ---
	// 表示やTD学習の棋譜記録用
	othello to_othello() const {
		othello board(8, vector<int>(8, 0));
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
				int bit = i * 8 + j;
				if (black & (1ULL << bit)) board[i][j] = 1;
				else if (white & (1ULL << bit)) board[i][j] = -1;
			}
		}
		return board;
	}

	// --- 合法手リスト（ビットマスク → 位置のリスト）---
	static vector<int> moves_to_list(uint64_t moves) {
		vector<int> result;
		while (moves) {
			// 最下位ビットを取り出す
			int pos = __builtin_ctzll(moves);
			result.push_back(pos);
			moves &= moves - 1;  // 最下位ビットを消す
		}
		return result;
	}
};

// --- BB版: Alpha-Beta探索 ---
// ビットボードを使った高速Minimax+Alpha-Beta。
// vector版のdfsと同じロジックだが、盤面コピーが軽量（int64×2）で高速。
int bb_dfs(int depth, bool is_black, BitBoard bb,
           int alpha, int beta, const int w[BOARD_SIZE][BOARD_SIZE], int max_depth) {
	if (depth == max_depth) return bb.evaluate(w);

	uint64_t legal = bb.get_legal_moves(is_black);

	// 合法手がない場合はパス
	if (legal == 0) {
		// 相手も置けなければゲーム終了: 石数で評価
		uint64_t opp_legal = bb.get_legal_moves(!is_black);
		if (opp_legal == 0) return bb.evaluate(w);
		// パスして相手に渡す
		return bb_dfs(depth, !is_black, bb, alpha, beta, w, max_depth);
	}

	// Move Ordering: weight値が大きい順にソート
	auto moves = BitBoard::moves_to_list(legal);
	sort(moves.begin(), moves.end(), [&w](int a, int b) {
		return w[a/8][a%8] > w[b/8][b%8];
	});

	if (!is_black) {
		// 先手●: スコア最大化（evaluateは後手有利⁽最大なので、先手は最小化したい）
		// …と思いきや、元のdfsと同じロジックを踏襲:
		// is_black=falseのときはmax (=先手番 → 先手がスコアを最大化…
		// いや、元のdfsのロジックをそのまま踏襲)
		int max_score = -INF;
		for (int pos : moves) {
			BitBoard next = bb.place(1ULL << pos, is_black);
			int score = bb_dfs(depth + 1, !is_black, next, alpha, beta, w, max_depth);
			max_score = max(max_score, score);
			alpha = max(alpha, score);
			if (alpha >= beta) break;
		}
		return max_score;
	} else {
		int min_score = INF;
		for (int pos : moves) {
			BitBoard next = bb.place(1ULL << pos, is_black);
			int score = bb_dfs(depth + 1, !is_black, next, alpha, beta, w, max_depth);
			min_score = min(min_score, score);
			beta = min(beta, score);
			if (alpha >= beta) break;
		}
		return min_score;
	}
}

// --- BB版: CPU手選択 ---
// 先手/後手どちらの側が打つかに応じて最善手を返す。
// 戻り値: 置く位置のビットインデックス(0～63), -1なら置けない
int bb_get_cpu_hand(bool is_black, BitBoard bb,
                    const int w[BOARD_SIZE][BOARD_SIZE], int search_depth) {
	uint64_t legal = bb.get_legal_moves(is_black);
	if (legal == 0) return -1;

	auto moves = BitBoard::moves_to_list(legal);
	int best_move = moves[0];
	int max_score = -INF;

	for (int pos : moves) {
		BitBoard next = bb.place(1ULL << pos, is_black);
		int t = bb_dfs(0, !is_black, next, -INF, INF, w, search_depth);
		// 後手○(is_black=false)はスコア最大化、先手●は反転して最大化
		int adjusted = is_black ? -t : t;
		if (adjusted > max_score) {
			best_move = pos;
			max_score = adjusted;
		}
	}
	return best_move;
}

// --- BB版: CPU vs CPU 1ゲーム ---
// 戻り値: 1=先手●勝ち, -1=後手○勝ち, 0=引き分け
int bb_play_game(
	const int w1[BOARD_SIZE][BOARD_SIZE], int depth1,
	const int w2[BOARD_SIZE][BOARD_SIZE], int depth2) {
	BitBoard bb = BitBoard::initial();
	bool is_black = true;  // 先手●から開始
	int pass_count = 0;

	while (true) {
		uint64_t legal = bb.get_legal_moves(is_black);
		if (legal == 0) {
			pass_count++;
			if (pass_count >= 2) break;
			is_black = !is_black;
			continue;
		}
		pass_count = 0;

		int pos;
		if (is_black) {
			pos = bb_get_cpu_hand(is_black, bb, w1, depth1);
		} else {
			pos = bb_get_cpu_hand(is_black, bb, w2, depth2);
		}

		bb = bb.place(1ULL << pos, is_black);
		is_black = !is_black;
	}
	return bb.judge();
}


// g++ -O2 -std=c++17 -o othello_test.exe "c++_learning\c++_learning.cpp"
//.\othello_test.exe

// === Phase 1: サイレントモード ===
bool SILENT_MODE = false;

//0:- , 1:● , -1:○ 
//int board[BOARD_SIZE][BOARD_SIZE] = {};
//マスごとの評価値
int val[BOARD_SIZE][BOARD_SIZE] = {};
int t_val[BOARD_SIZE][BOARD_SIZE] = {};
int weight[BOARD_SIZE][BOARD_SIZE] = {
	2714,  147,   69,  -18,  -18,   69,  147, 2714,
	 147, -577, -186, -153, -153, -186, -577,  147,
	  69, -186, -379, -122, -122, -379, -186,   69,
	 -18, -153, -122, -169, -169, -122, -153,  -18,
	 -18, -153, -122, -169, -169, -122, -153,  -18,
	  69, -186, -379, -122, -122, -379, -186,   69,
	 147, -577, -186, -153, -153, -186, -577,  147,
	2714,  147,   69,  -18,  -18,   69,  147, 2714
};

// === Phase 4: 学習用double weight ===
// int weightは対戦時にそのまま使う。
// 学習ではweightを微小量ずつ更新するためdouble精度が必要。
// 例: weight_d[0][0] = 2714.0 → 学習後 2714.32 のように小数点以下も保持する。
double weight_d[BOARD_SIZE][BOARD_SIZE];

// int weight → double weight_d にコピー
void weight_to_double() {
	rep(i, BOARD_SIZE) rep(j, BOARD_SIZE) {
		weight_d[i][j] = (double)weight[i][j];
	}
}

// double weight_d → int weight にコピー（四捨五入）
// 学習結果を対戦用に反映するときに使う
void double_to_weight() {
	rep(i, BOARD_SIZE) rep(j, BOARD_SIZE) {
		weight[i][j] = (int)(weight_d[i][j] + 0.5);
	}
}

// === Phase 1: weightのファイル保存・読み込み（int版） ===
void save_weight(const string& filename) {
	ofstream ofs(filename);
	if (!ofs) {
		cerr << "Error: Cannot open file for writing: " << filename << endl;
		return;
	}
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			if (j > 0) ofs << " ";
			ofs << weight[i][j];
		}
		ofs << "\n";
	}
	ofs.close();
	if (!SILENT_MODE) {
		cout << "Weight saved to " << filename << endl;
	}
}

void load_weight(const string& filename) {
	ifstream ifs(filename);
	if (!ifs) {
		cerr << "Error: Cannot open file for reading: " << filename << endl;
		return;
	}
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			ifs >> weight[i][j];
		}
	}
	ifs.close();
	if (!SILENT_MODE) {
		cout << "Weight loaded from " << filename << endl;
	}
}

// === Phase 4: double weightのファイル保存・読み込み ===
// 学習途中のweightを小数点精度で保存する
void save_weight_d(const string& filename) {
	ofstream ofs(filename);
	if (!ofs) {
		cerr << "Error: Cannot open file for writing: " << filename << endl;
		return;
	}
	// 小数点以下2桁で保存
	ofs << fixed;
	ofs.precision(2);
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			if (j > 0) ofs << " ";
			ofs << weight_d[i][j];
		}
		ofs << "\n";
	}
	ofs.close();
}

void load_weight_d(const string& filename) {
	ifstream ifs(filename);
	if (!ifs) {
		cerr << "Error: Cannot open file for reading: " << filename << endl;
		return;
	}
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			ifs >> weight_d[i][j];
		}
	}
	ifs.close();
}

//盤面生成
void make_board(othello& board) {
	board[3][3] = 1;
	board[4][4] = 1;
	board[3][4] = -1;
	board[4][3] = -1;
}


//盤面表示
void show_board(const othello& board) {
	if (SILENT_MODE) return;
	cout << "---------------------------" << endl;
	cout << "   1 2 3 4 5 6 7 8 ";
	rep(j, 8) printf("%2d", j+1);
	cout << endl;
	char t='a';
	int bcount = 0, wcount = 0;
	rep(i, BOARD_SIZE) {
		cout << char(t + i)<< " ";
		rep(j, BOARD_SIZE) {
			int t = board[i][j];
			if (t == 0)cout << " -";
			else if (t == 1) {
				cout << " ●";
				wcount++;
			}
			else if (t == -1) {
				cout << " ○";
				bcount++;
			}
		}
		cout << endl;
	}
	cout << "先手(●):" << wcount << endl;
	cout << "後手(○):" << bcount << endl;
}

//手番表示
void show_player(bool is_AI_turn) {
	if (SILENT_MODE) return;
	if (!is_AI_turn) {
		cout << "先手(●)の番です" << endl;
	}
	else {
		cout << "後手(○)の番です" << endl;
	}
}

void input_check(char* x, char* y) {
	//xが英字 yが数字になるようにする
	if (*x >= '1' && *x <= '9') {
		swap(*x, *y);
	}
	else {
		return;
	}
}

bool inside_board(int i, int j) {
	if (i < 0 || i >= BOARD_SIZE) {
		return false;
	}
	else if (j < 0 || j >= BOARD_SIZE) {
		return false;
	}
	return true;
}

bool check_puttable(bool is_AI_turn, int i, int j, const othello& board) {
	//盤面の範囲内かどうか
	if (!inside_board(i,j)) {
		return false;
	}

	//有効なマスか
	if (board[i][j] != 0) {
		return false;
	}
	//1:● -1:○ プレイヤー：1 ,AI:-1
	int player = is_AI_turn ? -1 : 1;
	//相手の駒を挟めるか
	for (int d_i = -1; d_i < 2; d_i++) {
		for (int d_j = -1; d_j < 2; d_j++) {
			if (d_i == 0 && d_j == 0) continue;
			//相手の駒が何個続くか
			int times = 1;

			//相手の駒が続いているか
			while (true) {
				int next_i = i + d_i * times;
				int next_j = j + d_j * times;

				//注目するマスが有効範囲内か
				if (!inside_board(next_i, next_j))break;
				//注目するマスが相手の駒じゃなかったら
				if (board[next_i][next_j] != player * -1) {
					break;
				}
				times++;
			}

			int next_i = i + d_i * times;
			int next_j = j + d_j * times;
			//注目するマスが有効範囲内か
			if (!inside_board(next_i, next_j))continue;
			//自分の駒で挟めていたら
			if (board[next_i][next_j] == player && times>1) {
				return true;
			}
		}
	}
	//そこには置けない
	return false;
}

//配置可能な座標をpairで返す
vector<pair<int, int>> get_puutable_places(bool is_AI_turn, const othello& board) {
	vector<pair<int, int>> ret;
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			if (check_puttable(is_AI_turn, i, j, board)) {
				ret.push_back({ i, j });
			}
		}
	}
	return ret;
}

// === Phase 3: Move Ordering（手の並べ替え） ===
// Alpha-Beta枝刈りは「良い手を先に探索する」と枝刈りが多発して高速化する。
// weightの値が大きいマス（角など）を先に探索することで、
// 早い段階でalpha/betaの値が更新され、不要な探索が大量にカットされる。
//
// 例: 角(weight=2714)を先に調べると alpha が大きくなり、
//     それ以降の弱い手(weight=-577等)は beta <= alpha で即カット
vector<pair<int, int>> get_puutable_places_ordered(
	bool is_AI_turn, const othello& board,
	const int w[BOARD_SIZE][BOARD_SIZE]) {
	// まず通常通り合法手を取得
	vector<pair<int, int>> ret = get_puutable_places(is_AI_turn, board);
	// weightが大きい順にソート（良い手を先に探索する）
	sort(ret.begin(), ret.end(), [&w](const pair<int,int>& a, const pair<int,int>& b) {
		return w[a.first][a.second] > w[b.first][b.second];
	});
	return ret;
}

//コマ設置
//AIの手を考える時用
int place_stn(bool is_AI_turn,int i, int j, othello& board) {
	int count = 0;
	int player = is_AI_turn ? -1 : 1;
	//挟んだ相手の駒を変更
	//相手の駒を挟めるか
	for (int d_i = -1; d_i < 2; d_i++) {
		for (int d_j = -1; d_j < 2; d_j++) {
			if (d_i == 0 && d_j == 0) continue;
			//相手の駒が何個続くか
			int times = 1;
			//相手の駒が続いているか
			while (true) {
				int next_i = i + d_i * times;
				int next_j = j + d_j * times;

				//注目するマスが有効範囲内か
				if (!inside_board(next_i, next_j))break;
				//注目するマスが相手の駒じゃなかったら
				if (board[next_i][next_j] != player * -1) {
					break;
				}
				times++;
			}

			int next_i = i + d_i * times;
			int next_j = j + d_j * times;
			//注目するマスが有効範囲内か
			if (!inside_board(next_i, next_j))continue;
			//自分の駒で挟めていたら
			if (board[next_i][next_j] == player && times > 1) {
				rep(k, times) {
					//駒を裏返していく
					board[i + d_i * k][j + d_j * k] = player;
					count++;
				}
				count--;
			}
		}
	}
	board[i][j] = player;
	return count;
}

// === Phase 1: 勝敗を戻り値で返す ===
// 戻り値: 1=先手(●)勝ち, -1=後手(○)勝ち, 0=引き分け
int judge_result(const othello& board) {
	int bcount = 0, wcount = 0;
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			if (board[i][j] == 1)wcount++;
			else if (board[i][j] == -1)bcount++;
		}
	}
	if (wcount > bcount) return 1;
	else if (bcount > wcount) return -1;
	else return 0;
}

void judge(const othello& board) {
	int bcount = 0, wcount = 0;
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			if (board[i][j] == 1)wcount++;
			else if (board[i][j] == -1)bcount++;
		}
	}
	cout << "先手(●)：" << wcount << "　後手(○)：" << bcount << endl;
	if (wcount > bcount)cout << "先手の勝利です" << endl;
	else if (bcount > wcount)cout << "後手の勝利です" << endl;
	else if (wcount == bcount)cout << "引き分けです" << endl;
}

bool flag_fin(bool& is_AI_turn, const othello& board) {
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			//どこかに置ける
			if (check_puttable(is_AI_turn, i, j, board)) {
				return true;
			}
		}
	}
	
	//プレイヤー交代で続行可能か
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			//どこかに置ける
			if (check_puttable(!is_AI_turn, i, j, board)) {
				if (!SILENT_MODE) cout << "パスしました" << endl;
				is_AI_turn = !is_AI_turn;
				return true;
			}
		}
	}
	if (!SILENT_MODE) cout << "どこにも置けません" << endl;
	judge(board);
	return false;
}

// === Phase 1: evaluate/dfs/get_AI_hand にweight引数を追加 ===

//考えられる手を引数に取る
int evaluate(bool is_AI_turn, othello& board, pair<int, int> hand,
             const int w[BOARD_SIZE][BOARD_SIZE]) {
	int i = hand.first, j = hand.second;
	place_stn(is_AI_turn,i, j, board);

	int score = 0;
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			// w[i][j] はマスの重み
			if (board[i][j] == -1) { // AIの石
				score += w[i][j];
			}
			else if (board[i][j] == 1) { // プレイヤーの石
				score -= w[i][j];
			}
		}
	}

	return score;
}

// === Phase 3: dfsにMove Orderingを適用 ===
// 変更点: get_puutable_places → get_puutable_places_ordered
// Alpha-Beta探索では、最初に調べる手が良い手であるほど枝刈りが効く。
// Move Orderingにより、weightが高いマス（角や辺）から探索するため、
// 早い段階で強いalpha/beta値が確定し、弱い手は探索せずにカットされる。
int dfs(int depth, bool is_AI_turn, pair<int, int> hand, othello& board,
        int alpha, int beta, const int w[BOARD_SIZE][BOARD_SIZE], int max_depth) {
	if (depth == max_depth) return evaluate(is_AI_turn, board, hand, w);
	othello next_board = board;
	place_stn(is_AI_turn, hand.first, hand.second, next_board);
	is_AI_turn = !is_AI_turn;

	// Move Ordering: weightが高い順に合法手をソートして探索
	auto ordered_hands = get_puutable_places_ordered(is_AI_turn, next_board, w);

	if (!is_AI_turn) {
		int max_score = -INF;
		for (auto next_hand : ordered_hands) {
			int score = dfs(depth + 1, is_AI_turn, next_hand, next_board, alpha, beta, w, max_depth);
			if (DEBUG_OPTION) {
				cout << "depth:" << depth << " 人の考えられる手は" << char('a' + next_hand.first) << 1 + next_hand.second << "でスコアは" << score << "です" << endl;
			}
			max_score = max(max_score, score);
			alpha = max(alpha, score);
			if (alpha >= beta) {
				break;  // β枝刈り: これ以上探索しても結果は変わらない
			}
		}
		return max_score;
	}
	else {
		int min_score = INF;
		for (auto next_hand : ordered_hands) {
			int score = dfs(depth + 1, is_AI_turn, next_hand, next_board, alpha, beta, w, max_depth);
			if (DEBUG_OPTION) {
				cout << "depth:" << depth << " 人の考えられる最悪の手は" << char('a' + next_hand.first) << 1 + next_hand.second << "でスコアは" << score << "です" << endl;
			}
			min_score = min(min_score, score);
			beta = min(beta, score);

			if (alpha >= beta) {
				break;  // α枝刈り: これ以上探索しても結果は変わらない
			}
		}
		return min_score;
	}
}


pair<int, int> get_AI_hand(othello& board,
                           const int w[BOARD_SIZE][BOARD_SIZE], int search_depth) {
	rep(i, BOARD_SIZE)rep(j, BOARD_SIZE) val[i][j] = 0;
	auto hands = get_puutable_places(true, board);
	pair<int, int> next_hand = hands[0];
	int max_score = -INF;
	int alpha = -INF;
	int beta = INF;
	for (auto hand : hands) {
		if (DEBUG_OPTION) {
			cout << "AIの手一周目：" << char('a' + hand.first) << 1 + hand.second << endl;
		}
		int t = dfs(0, true, hand, board, alpha, beta, w, search_depth);
		if (t > max_score) {
			next_hand = hand;
			max_score = t;
		}
	}
	if (!SILENT_MODE) {
		cout << "AIの手：" << char('a' + next_hand.first) << 1 + next_hand.second << endl;
		cout << "-------------------------------------" << endl;
	}
	return next_hand;
}

// === Phase 2: 汎用CPU手選択（先手・後手両対応） ===
// is_AI_turn: このCPUがAI側(後手○=-1)かどうか
pair<int, int> get_cpu_hand(bool is_AI_turn, othello& board,
                            const int w[BOARD_SIZE][BOARD_SIZE], int search_depth) {
	rep(i, BOARD_SIZE)rep(j, BOARD_SIZE) val[i][j] = 0;
	auto hands = get_puutable_places(is_AI_turn, board);
	if (hands.empty()) return { -1, -1 };
	pair<int, int> next_hand = hands[0];
	int max_score = -INF;
	int alpha = -INF;
	int beta = INF;
	for (auto hand : hands) {
		// dfsは「AIの石(後手○)が有利なほどスコアが高い」前提なので、
		// 先手CPUの場合はスコアを反転して最大化する
		int t = dfs(0, is_AI_turn, hand, board, alpha, beta, w, search_depth);
		int adjusted = is_AI_turn ? t : -t;
		if (adjusted > max_score) {
			next_hand = hand;
			max_score = adjusted;
		}
	}
	if (!SILENT_MODE) {
		string side = is_AI_turn ? "後手(○)" : "先手(●)";
		cout << side << "CPUの手：" << char('a' + next_hand.first) << 1 + next_hand.second << endl;
	}
	return next_hand;
}

// === Phase 2: CPU vs CPU 1ゲーム実行 ===
// 戻り値: 1=先手(●)勝ち, -1=後手(○)勝ち, 0=引き分け
int play_game_cpu_vs_cpu(
	const int w1[BOARD_SIZE][BOARD_SIZE], int depth1,  // 先手●のweight・探索深さ
	const int w2[BOARD_SIZE][BOARD_SIZE], int depth2   // 後手○のweight・探索深さ
) {
	othello board(BOARD_SIZE, vector<int>(BOARD_SIZE, 0));
	make_board(board);

	int player = 1;  // 1=先手●, -1=後手○
	bool is_AI_turn = false;  // 先手●から開始
	int pass_count = 0;

	while (true) {
		auto hands = get_puutable_places(is_AI_turn, board);
		if (hands.empty()) {
			pass_count++;
			if (pass_count >= 2) break;  // 両者パス → 終了
			player *= -1;
			is_AI_turn = !is_AI_turn;
			continue;
		}
		pass_count = 0;

		pair<int, int> hand;
		if (player == 1) {
			// 先手CPU
			hand = get_cpu_hand(is_AI_turn, board, w1, depth1);
		} else {
			// 後手CPU
			hand = get_cpu_hand(is_AI_turn, board, w2, depth2);
		}

		place_stn(is_AI_turn, hand.first, hand.second, board);
		show_board(board);

		player *= -1;
		is_AI_turn = !is_AI_turn;
	}

	show_board(board);
	int result = judge_result(board);
	if (!SILENT_MODE) {
		judge(board);
	}
	return result;
}

// ============================================================
// === Phase 4: 強化学習 (TD学習) の中核部分 ===
// ============================================================

// --- GameRecord: 1ゲームの棋譜（全盤面の履歴 + 勝敗結果）---
// TD学習では「各手番の盤面評価値がどう推移したか」を使って学習する。
// そのため、ゲーム中の全ての盤面をboardsに記録しておく必要がある。
struct GameRecord {
	vector<othello> boards;  // 各手番の盤面スナップショット
	int result;              // 1=先手●勝ち, -1=後手○勝ち, 0=引き分け
};

// --- double版の盤面評価関数 ---
// int版のevaluateと同じロジックだが、weight_dを使ってdouble精度で計算する。
// 後手○(AI)が有利なほどスコアが高くなる。
double evaluate_d(const othello& board) {
	double score = 0.0;
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			// board[i][j]:  1=先手●の石, -1=後手○の石, 0=空
			// 後手(AI)の石 → スコア加算 / 先手の石 → スコア減算
			if (board[i][j] == -1) {
				score += weight_d[i][j];
			}
			else if (board[i][j] == 1) {
				score -= weight_d[i][j];
			}
		}
	}
	return score;
}

// --- TD(Temporal Difference)学習によるweight更新 ---
//
// 【TD学習とは？】
// ゲーム中の「連続する2つの盤面」の評価値の差 (TD誤差) を使って
// weightを修正する手法。
//
// 考え方:
//   盤面 s_t の評価値 V(s_t) は、次の盤面 s_{t+1} の評価値 V(s_{t+1}) に
//   近づくべき。なぜなら、次の盤面のほうが「真の結果」に近い情報を持つから。
//
// 更新式:
//   weight_d[i][j] += 学習率 × TD誤差 × 特徴量
//   TD誤差 = V(s_{t+1}) - V(s_t)    ← 次の盤面と今の盤面の評価値の差
//   特徴量 = board[i][j]             ← そのマスに石があるか (-1, 0, 1)
//
// 最終手番では「TD誤差 = 実際の勝敗報酬 - V(s_t)」を使う。
// 勝敗報酬: 後手○勝ち→+1000, 先手●勝ち→-1000, 引分→0
void td_update(const GameRecord& record, double learning_rate) {
	int T = record.boards.size();
	if (T < 2) return;  // 盤面が2つ未満では学習できない

	for (int t = 0; t < T - 1; t++) {
		// 今の盤面と次の盤面の評価値を計算
		double v_current = evaluate_d(record.boards[t]);
		double v_next = evaluate_d(record.boards[t + 1]);

		// TD誤差: 次の盤面の評価値と今の盤面の評価値の差
		// この差分の方向にweightを修正することで、
		// 予測精度が徐々に向上していく
		double td_error = v_next - v_current;

		// 最終手番では実際の勝敗結果を「教師信号」として使う
		// これにより、ゲームの勝敗情報が全体のweight更新に反映される
		if (t == T - 2) {
			// result: 1=先手勝ち, -1=後手勝ち, 0=引分
			// 後手(AI)視点で評価するため -1 を掛ける
			// → 後手勝ち: +1000, 先手勝ち: -1000
			double final_reward = (double)record.result * -1000.0;
			td_error = final_reward - v_current;
		}

		// weightの更新: 各マスについて
		//   weight_d[i][j] += 学習率 × TD誤差 × 特徴量(board[i][j])
		//
		// 特徴量 board[i][j] の意味:
		//   +1 (先手●の石がある) → TD誤差に比例してweightを増やす
		//   -1 (後手○の石がある) → TD誤差に比例してweightを減らす
		//    0 (空マス)          → 何も更新しない
		rep(i, BOARD_SIZE) {
			rep(j, BOARD_SIZE) {
				double feature = (double)record.boards[t][i][j];
				weight_d[i][j] += learning_rate * td_error * feature;
			}
		}
	}
}

// --- 対称性の適用 ---
// オセロ盤は8方向に対称（90度回転×4 × 左右反転×2 = 8通り）。
// つまり、角 (0,0) と (0,7) と (7,0) と (7,7) は全て「角」という
// 同じ意味を持つので、同じweight値であるべき。
//
// 学習中はノイズで対称性が崩れるため、定期的に
// 対称位置のweightを平均化して対称性を回復する。
// これにより独立パラメータ数が実質10個程度に圧縮され、
// 学習効率と安定性が大幅に向上する。
void apply_symmetry() {
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			// 8方向の対称位置のweightを全て集めて平均化
			// (i,j) の対称位置:
			//   回転0:   (i,j)      回転90:  (j,7-i)
			//   回転180: (7-i,7-j)  回転270: (7-j,i)
			//   反転後: 上記4つのi,jを入れ替えたもの
			double sum = weight_d[i][j]
			           + weight_d[i][7-j]
			           + weight_d[7-i][j]
			           + weight_d[7-i][7-j]
			           + weight_d[j][i]
			           + weight_d[j][7-i]
			           + weight_d[7-j][i]
			           + weight_d[7-j][7-i];
			double avg = sum / 8.0;

			// 全ての対称位置に同じ平均値を設定
			weight_d[i][j] = avg;
			weight_d[i][7-j] = avg;
			weight_d[7-i][j] = avg;
			weight_d[7-i][7-j] = avg;
			weight_d[j][i] = avg;
			weight_d[j][7-i] = avg;
			weight_d[7-j][i] = avg;
			weight_d[7-j][7-i] = avg;
		}
	}
}

// --- 棋譜を記録しながらCPU対戦を行う ---
// play_game_cpu_vs_cpuと同じロジックだが、
// 各手番の盤面をGameRecordに記録して返す。
// この棋譜がTD学習の入力データになる。
GameRecord play_and_record(int search_depth) {
	GameRecord record;
	othello board(BOARD_SIZE, vector<int>(BOARD_SIZE, 0));
	make_board(board);

	// 初期盤面を記録
	record.boards.push_back(board);

	int player = 1;
	bool is_AI_turn = false;
	int pass_count = 0;

	while (true) {
		auto hands = get_puutable_places(is_AI_turn, board);
		if (hands.empty()) {
			pass_count++;
			if (pass_count >= 2) break;
			player *= -1;
			is_AI_turn = !is_AI_turn;
			continue;
		}
		pass_count = 0;

		// 現在のweight(int)を使ってCPUが手を選択
		pair<int, int> hand = get_cpu_hand(is_AI_turn, board, weight, search_depth);
		place_stn(is_AI_turn, hand.first, hand.second, board);

		// 手を打った後の盤面を記録（TD学習で使用する）
		record.boards.push_back(board);

		player *= -1;
		is_AI_turn = !is_AI_turn;
	}

	record.result = judge_result(board);
	return record;
}

// --- BB版: 棋譜記録付きCPU対戦（TD学習用）---
// BB版の高速探索でCPU対戦し、各手番の盤面をothello形式で記録。
// verbose=trueの場合、各手番の盤面と手を表示する。
GameRecord bb_play_and_record(int search_depth, bool verbose = false) {
	GameRecord record;
	BitBoard bb = BitBoard::initial();

	// 初期盤面を記録
	record.boards.push_back(bb.to_othello());

	if (verbose) {
		cout << "--- 対戦開始 ---" << endl;
		bool save_silent = SILENT_MODE;
		SILENT_MODE = false;
		show_board(bb.to_othello());
		SILENT_MODE = save_silent;
	}

	bool is_black = true;
	int pass_count = 0;
	int move_num = 0;

	while (true) {
		uint64_t legal = bb.get_legal_moves(is_black);
		if (legal == 0) {
			pass_count++;
			if (pass_count >= 2) break;
			if (verbose) {
				cout << (is_black ? "先手●" : "後手○") << " パス" << endl;
			}
			is_black = !is_black;
			continue;
		}
		pass_count = 0;

		int pos = bb_get_cpu_hand(is_black, bb, weight, search_depth);
		bb = bb.place(1ULL << pos, is_black);
		move_num++;

		// 盤面をothello形式で記録（TD学習用）
		record.boards.push_back(bb.to_othello());

		if (verbose) {
			int row = pos / 8, col = pos % 8;
			printf("%2d手目: %s %c%d\n", move_num,
				is_black ? "先手●" : "後手○",
				'a' + row, col + 1);
			bool save_silent = SILENT_MODE;
			SILENT_MODE = false;
			show_board(bb.to_othello());
			SILENT_MODE = save_silent;
		}

		is_black = !is_black;
	}

	record.result = bb.judge();

	if (verbose) {
		int b = bb.count_black(), w = bb.count_white();
		printf("--- 結果: ●%d - ○%d ", b, w);
		if (record.result == 1) cout << "先手●勝ち" << endl;
		else if (record.result == -1) cout << "後手○勝ち" << endl;
		else cout << "引き分け" << endl;
		cout << endl;
	}

	return record;
}

// --- weight_dの内容を表示（デバッグ用）---
void show_weight_d() {
	cout << "--- weight_d ---" << endl;
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			printf("%8.1f", weight_d[i][j]);
		}
		cout << endl;
	}
}

// ============================================================
// === Phase 5: 学習ループ & Weight保存 ===
// ============================================================
//
// 学習の全体フロー:
//   1. 初期weightを保存
//   2. 指定エポック数ループ:
//      a. N試合の自己対戦 (棋譜記録)
//      b. TD学習でweight_d更新
//      c. 対称性の回復
//      d. int weightに反映
//      e. 定期保存 & 進捗表示
//   3. 最終weightを保存
//   4. 初期weightとの対戦で学習効果を検証

void train(int epochs, int games_per_epoch, double learning_rate, int save_interval) {
	auto total_start = chrono::high_resolution_clock::now();

	// --- 初期weightを保存（学習前との比較用）---
	int initial_weight[BOARD_SIZE][BOARD_SIZE];
	rep(i, BOARD_SIZE) rep(j, BOARD_SIZE) {
		initial_weight[i][j] = weight[i][j];
	}
	save_weight("weight_initial.txt");

	// int weight → double weight_d にコピー
	weight_to_double();

	cout << "=== 学習開始 ===" << endl;
	cout << "学習率: " << learning_rate << endl;
	cout << "探索深さ: " << LEARNING_DEPTH << endl;
	cout << "エポック数: " << epochs << endl;
	cout << "1エポックあたり対戦数: " << games_per_epoch << endl;
	cout << "保存間隔: " << save_interval << "エポックごと" << endl;
	cout << endl;

	// --- 学習ループ ---
	for (int epoch = 0; epoch < epochs; epoch++) {
		auto epoch_start = chrono::high_resolution_clock::now();
		int wins = 0, losses = 0, draws = 0;

		for (int game = 0; game < games_per_epoch; game++) {
			// 各エポックの最初の1試合だけ盤面を表示
			bool verbose = (game == 0);
			GameRecord record = bb_play_and_record(LEARNING_DEPTH, verbose);

			// TD学習: 棋譜からweight_dを更新
			td_update(record, learning_rate);

			if (record.result == 1) wins++;
			else if (record.result == -1) losses++;
			else draws++;
		}

		// 対称性を回復
		apply_symmetry();

		// 学習結果をint weightに反映（次のゲームで使う）
		double_to_weight();

		auto epoch_end = chrono::high_resolution_clock::now();
		auto epoch_ms = chrono::duration_cast<chrono::milliseconds>(epoch_end - epoch_start).count();

		// --- 進捗表示 ---
		printf("Epoch %4d/%d | W=%3d D=%3d L=%3d | 角=%.1f | %ldms\n",
			epoch + 1, epochs, wins, draws, losses, weight_d[0][0], epoch_ms);

		// --- 定期保存 ---
		if ((epoch + 1) % save_interval == 0) {
			string filename = "weight_epoch_" + to_string(epoch + 1) + ".txt";
			save_weight(filename);
			save_weight_d("weight_d_epoch_" + to_string(epoch + 1) + ".txt");
			cout << "  >> weight保存: " << filename << endl;
		}
	}

	// --- 最終weightを保存 ---
	save_weight("weight_final.txt");
	save_weight_d("weight_d_final.txt");

	auto total_end = chrono::high_resolution_clock::now();
	auto total_sec = chrono::duration_cast<chrono::seconds>(total_end - total_start).count();

	cout << endl;
	cout << "=== 学習完了 ===" << endl;
	cout << "総時間: " << total_sec << "秒" << endl;
	cout << "最終weight保存先: weight_final.txt / weight_d_final.txt" << endl;
	cout << endl;

	// --- 学習前後のweight比較 ---
	cout << "=== 学習前後のweight変化 ===" << endl;
	cout << "マス      | 学習前 | 学習後 | 変化量" << endl;
	cout << "------+------+------+------" << endl;
	// 代表的なマスだけ表示
	const int spots[][2] = {{0,0},{0,1},{0,2},{0,3},{1,1},{1,2},{1,3},{2,2},{2,3},{3,3}};
	const char* names[] = {"角   ", "角隣C", "角隣B", "辺3 ", "角隣X", "角隣 ", "辺2 ", "中央近", "中央近", "中央 "};
	for (int k = 0; k < 10; k++) {
		int i = spots[k][0], j = spots[k][1];
		printf("%s | %5d | %5d | %+.1f\n",
			names[k], initial_weight[i][j], weight[i][j],
			weight_d[i][j] - initial_weight[i][j]);
	}
	cout << endl;

	// --- 学習済weight vs 初期weight で対戦評価 ---
	cout << "=== 学習済 vs 初期weight (100試合) ===" << endl;
	int eval_wins = 0, eval_losses = 0, eval_draws = 0;
	for (int i = 0; i < 100; i++) {
		// 先手: 学習済, 後手: 初期
		int r = bb_play_game(weight, LEARNING_DEPTH, initial_weight, LEARNING_DEPTH);
		if (r == 1) eval_wins++;
		else if (r == -1) eval_losses++;
		else eval_draws++;
	}
	printf("学習済(先手) vs 初期(後手): W=%d D=%d L=%d\n", eval_wins, eval_draws, eval_losses);

	// 先後入れ替え
	eval_wins = 0; eval_losses = 0; eval_draws = 0;
	for (int i = 0; i < 100; i++) {
		// 先手: 初期, 後手: 学習済
		int r = bb_play_game(initial_weight, LEARNING_DEPTH, weight, LEARNING_DEPTH);
		if (r == 1) eval_losses++;   // 初期が勝ち = 学習済の負け
		else if (r == -1) eval_wins++; // 学習済が勝ち
		else eval_draws++;
	}
	printf("初期(先手) vs 学習済(後手): W=%d D=%d L=%d\n", eval_wins, eval_draws, eval_losses);
}

// === 人間 vs AI の対戦 ===
void play_human_vs_ai() {
	vector<vector<int>> board(BOARD_SIZE, vector<int>(BOARD_SIZE));
	make_board(board);
	int turn = 0;
	bool flag = false;
	int player = 1;
	bool is_AI_turn = player == -1;

	while (flag_fin(is_AI_turn, board)) {
		show_board(board);
		show_player(is_AI_turn);
		int x, y;
		if (get_puutable_places(is_AI_turn, board).empty()) {
			if (flag) break;
			flag = true;
			continue;
		}
		if (player == 1) {
			char tx, ty;
			do {
				cout << "配置する座標を入力してください" << endl;
				cin >> tx >> ty;
				input_check(&tx, &ty);
				x = tx - 'a';
				y = ty - '1';
			} while (!check_puttable(is_AI_turn, x, y, board));
		}
		else if (player == -1) {
			cout << "CPU Thinking..." << endl;
			auto hand = get_AI_hand(board, weight, DEPTH);
			x = hand.first, y = hand.second;
		}
		place_stn(is_AI_turn, x, y, board);
		player *= -1;
		is_AI_turn = !is_AI_turn;
	}
	show_board(board);
	judge(board);
}

int main() {
	cout << "=== オセロ ===" << endl;
	cout << "1: 人間 vs AI" << endl;
	cout << "2: CPU vs CPU" << endl;
	cout << "3: ベンチマーク" << endl;
	cout << "4: TD学習テスト（5エポック）" << endl;
	cout << "5: 本格学習" << endl;
	cout << "6: 学習済weightで人間 vs AI" << endl;
	cout << "モードを選択してください: ";

	int mode;
	cin >> mode;

	if (mode == 1) {
		play_human_vs_ai();
	}
	else if (mode == 2) {
		cout << "CPU vs CPU 対戦を開始します" << endl;
		int result = play_game_cpu_vs_cpu(weight, DEPTH, weight, DEPTH);
		cout << "=== 結果: ";
		if (result == 1) cout << "先手(●)の勝利" << endl;
		else if (result == -1) cout << "後手(○)の勝利" << endl;
		else cout << "引き分け" << endl;
	}
	else if (mode == 3) {
		SILENT_MODE = true;
		cout << "=== ベンチマーク開始 (BitBoard版) ===" << endl;

		auto start5 = chrono::high_resolution_clock::now();
		int result5 = bb_play_game(weight, DEPTH, weight, DEPTH);
		auto end5 = chrono::high_resolution_clock::now();
		auto ms5 = chrono::duration_cast<chrono::milliseconds>(end5 - start5).count();

		auto start3 = chrono::high_resolution_clock::now();
		int result3 = bb_play_game(weight, LEARNING_DEPTH, weight, LEARNING_DEPTH);
		auto end3 = chrono::high_resolution_clock::now();
		auto ms3 = chrono::duration_cast<chrono::milliseconds>(end3 - start3).count();

		cout << "[BitBoard] 深さ" << DEPTH << ": " << ms5 << "ms (結果:" << result5 << ")" << endl;
		cout << "[BitBoard] 深さ" << LEARNING_DEPTH << ": " << ms3 << "ms (結果:" << result3 << ")" << endl;
		if (ms3 > 0) {
			cout << "高速化倍率: " << (double)ms5 / ms3 << "倍" << endl;
		}
		cout << endl;

		int num_games = 10;
		auto start_batch = chrono::high_resolution_clock::now();
		int wins = 0, losses = 0, draws = 0;
		for (int i = 0; i < num_games; i++) {
			int r = bb_play_game(weight, LEARNING_DEPTH, weight, LEARNING_DEPTH);
			if (r == 1) wins++;
			else if (r == -1) losses++;
			else draws++;
		}
		auto end_batch = chrono::high_resolution_clock::now();
		auto ms_batch = chrono::duration_cast<chrono::milliseconds>(end_batch - start_batch).count();

		cout << "=== [BitBoard] 深さ" << LEARNING_DEPTH << "で" << num_games << "試合 ===" << endl;
		cout << "合計: " << ms_batch << "ms" << endl;
		cout << "1試合平均: " << ms_batch / num_games << "ms" << endl;
		cout << "先手勝ち:" << wins << " 後手勝ち:" << losses << " 引分:" << draws << endl;
	}
	else if (mode == 4) {
		SILENT_MODE = true;
		cout << "=== TD学習テスト開始 ===" << endl;
		weight_to_double();
		double learning_rate = 0.001;
		int epochs = 5;
		int games_per_epoch = 10;

		cout << "学習率: " << learning_rate << endl;
		cout << "学習前の角weight: " << weight_d[0][0] << endl;

		for (int epoch = 0; epoch < epochs; epoch++) {
			int wins = 0, losses = 0, draws = 0;
			for (int game = 0; game < games_per_epoch; game++) {
				GameRecord record = bb_play_and_record(LEARNING_DEPTH);
				td_update(record, learning_rate);
				if (record.result == 1) wins++;
				else if (record.result == -1) losses++;
				else draws++;
			}
			apply_symmetry();
			double_to_weight();
			printf("Epoch %d/%d: W=%d D=%d L=%d | 角weight=%.1f\n",
				epoch + 1, epochs, wins, draws, losses, weight_d[0][0]);
		}
		cout << endl;
		show_weight_d();
	}
	// === Phase 5: 本格学習モード ===
	else if (mode == 5) {
		SILENT_MODE = true;
		int epochs, games_per_epoch, save_interval;
		double learning_rate;

		cout << "エポック数 (推奨: 500): ";
		cin >> epochs;
		cout << "1エポックあたり対戦数 (推奨: 50): ";
		cin >> games_per_epoch;
		cout << "学習率 (推奨: 0.001): ";
		cin >> learning_rate;
		cout << "保存間隔 (何エポックごと, 推奨: 100): ";
		cin >> save_interval;

		train(epochs, games_per_epoch, learning_rate, save_interval);
	}
	// === Phase 5: 学習済weightで人間 vs AI ===
	else if (mode == 6) {
		string filename;
		cout << "weightファイル名 (weight_final.txt等): ";
		cin >> filename;
		load_weight(filename);
		cout << "学習済weightで対戦します" << endl;
		play_human_vs_ai();
	}
	else {
		cout << "無効なモードです" << endl;
	}

	return 0;
}
