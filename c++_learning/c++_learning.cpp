#include<iostream>
#include<string>
#include<vector>

#define rep(i,n) for(int i=0;i<n;i++)
#define BOARD_SIZE 8
#define DEPTH 5
#define othello std::vector<std::vector<int>>
#define INF 99999
#define DEBUG_OPTION false

using namespace std;
using std::cout;

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

//盤面生成
void make_board(othello& board) {
	board[3][3] = 1;
	board[4][4] = 1;
	board[3][4] = -1;
	board[4][3] = -1;
}


//盤面表示
void show_board(const othello& board) {
	cout << "---------------------------" << endl;
	cout << "   1 2 3 4 5 6 7 8 " ;
	rep(j, 8) printf("%2d", j+1);
	cout << endl;
	char t='a';
	int bcount = 0, wcount = 0;
	rep(i, BOARD_SIZE) {
		cout << char(t + i)<<" ";
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
		//cout << " ";
		//rep(j, BOARD_SIZE) {
		//	printf("%2d",val[i][j]);
		//}
		cout << endl;
	}
	cout << "先手(●):" << wcount << endl;
	cout << "後手(○):" << bcount << endl;
}

//手番表示
void show_player(bool is_AI_turn) {
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
			//cout << "debug check: i=" <<next_i<<" j:"<<next_j<< endl;
			//注目するマスが有効範囲内か
			if (!inside_board(next_i, next_j))continue;
			//cout << "debug check" << endl;
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
				cout << "パスしました" << endl;
				is_AI_turn = !is_AI_turn;
				return true;
			}
		}
	}
	cout << "どこにも置けません" << endl;
	judge(board);
	return false;
}

//AIの次の手を考える
/*
void AIturn(const othello& board) {
	int count = 0;
	pair<int, int> point;
	int max_val = 0;
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			val[i][j] = 0;
			if (check_puttable(i, j, board)) {
				int a = place_stn(i, j, board);
				val[i][j] = a;
				if (a > count) {
					count = a;
					//point = { i,j };
				}
				//辺の上だったら
				if (i == 0 || i == BOARD_SIZE-1) {
					//四隅だったら
					if (j == 0 || j == BOARD_SIZE-1) {
						val[i][j] = 99;
					}
					//すぐに取られるならやめておく
					//else if (board[i][j + 1] == 1) {
					//	val[i][j] = -1;
					//}
					//else if (board[i][j - 1] == 1) {
					//	val[i][j] = -1;
					//}
					else val[i][j] = 9;
				}
				else if (j == 0 || j == BOARD_SIZE-1) {
					//すぐに取られるならやめておく
					if (board[i + 1][j] == 1) {
						val[i][j] = -1;
					}
					else if (board[i - 1][j] == 1) {
						val[i][j] = -1;
					}
					else val[i][j] = 9;
				}
				if (val[i][j] > max_val) {
					max_val = val[i][j];
					point = { i,j };
				}
			}
		}
	}
	cout << "AIのターンです：" << char('a'  + point.first) <<point.second+1 << endl;
	place_stn(point.first, point.second, false);
}
*/

//考えられる手を引数に取る
int evaluate(bool is_AI_turn, othello& board, pair<int, int> hand) {
	//int t_player = is_AI_turn ? -1 : 1;
	int i = hand.first, j = hand.second;
	place_stn(is_AI_turn,i, j, board);

	int score = 0;
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			// weight[i][j] はマスの重み
			if (board[i][j] == -1) { // AIの石
				score += weight[i][j];
			}
			else if (board[i][j] == 1) { // プレイヤーの石
				score -= weight[i][j];
			}
		}
	}

	return score;
}

int dfs(int depth, bool is_AI_turn, pair<int, int> hand, othello& board, int alpha, int beta) {
	if (depth == DEPTH) return evaluate(is_AI_turn, board, hand);
	othello next_board = board;
	place_stn(is_AI_turn, hand.first, hand.second, next_board);
	is_AI_turn = !is_AI_turn;

	if (!is_AI_turn) {
		int max_score = -INF;
		for (auto next_hand : get_puutable_places(is_AI_turn, next_board)) {
			int score = dfs(depth + 1, is_AI_turn, next_hand, next_board, alpha, beta);
			if (DEBUG_OPTION) {
				cout << "depth:" << depth << " 人の考えられる手は" << char('a' + next_hand.first) << 1 + next_hand.second << "でスコアは" << score << "です" << endl;
			}
			max_score = max(max_score, score);
			alpha = max(alpha, score);
			if (alpha >= beta) {
				break;
			}
		}
		return max_score;
	}
	else {
		int min_score = INF;
		for (auto next_hand : get_puutable_places(is_AI_turn, next_board)) { 
			int score = dfs(depth + 1, is_AI_turn, next_hand, next_board, alpha, beta);
			if (DEBUG_OPTION) {
				cout << "depth:" << depth << " 人の考えられる最悪の手は" << char('a' + next_hand.first) << 1 + next_hand.second << "でスコアは" << score << "です" << endl;
			}
			min_score = min(min_score, score);
			beta = min(beta, score);

			if (alpha >= beta) {
				break;
			}
		}
		return min_score;
	}
}


pair<int, int> get_AI_hand(othello& board) {
	//dfs(0, true, hand, board);
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
		int t = dfs(0, true, hand, board, alpha, beta);
		//val[hand.first][hand.second] = t;
		if (t > max_score) {
			next_hand = hand;
			max_score = t;
		}
	}
	cout << "AIの手：" << char('a' + next_hand.first) << 1 + next_hand.second << endl;
	cout << "-------------------------------------" << endl;
	return next_hand;
}

int main() {
	vector<vector<int>> board(BOARD_SIZE,vector<int>(BOARD_SIZE));
	make_board(board);
	char a = 'a';
	int turn = 0;
	bool flag = false;
	//1:● -1:○ プレイヤー：1 ,AI:-1
	int player = 1;
	bool is_AI_turn = player == -1;

	while (flag_fin(is_AI_turn,board)) {
		show_board(board);
		show_player(is_AI_turn);
		int x, y;
		if (get_puutable_places(is_AI_turn, board).empty()) {
			if (flag)break;
			flag = true;
			continue;
		}
		if (player == 1) {
			char tx, ty;
			do {
				//x行目y列目
				//英字が行　数字が列
				cout << "配置する座標を入力してください" << endl;
				cin >> tx >> ty;
				input_check(&tx, &ty);
				//数値に変換
				x = tx - 'a';
				y = ty - '1';
			} while (!check_puttable(is_AI_turn, x, y, board));
			//cout << x << "行" << y << "列目" << endl
			//cout << "player:" << player << endl;
		}
		else if (player == -1) {
			cout << "CPU Thinking..." << endl;
			//AIの手を実行
			auto hand = get_AI_hand(board);
			x = hand.first, y = hand.second;
		}
		place_stn(is_AI_turn, x, y, board);
		player *= -1;
		is_AI_turn = !is_AI_turn;
		//cout << "turn:" << ++turn << endl;
	}
	show_board(board);
	judge(board);
	return 0;
}
