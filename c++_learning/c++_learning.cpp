#include<iostream>
#include<string>
#include<vector>

#define rep(i,n) for(int i=0;i<n;i++)
#define BOARD_SIZE 8
#define DEPTH 3

using namespace std;

//0:- , 1:● , -1:○ 
int board[BOARD_SIZE][BOARD_SIZE] = {};
int t_board[BOARD_SIZE][BOARD_SIZE] = {};
//マスごとの評価値
int val[BOARD_SIZE][BOARD_SIZE] = {};
int weitht[BOARD_SIZE][BOARD_SIZE] = {
	99,-20,-20,20,20,-20,-20,99,
	-20,-20,1,1,1,1,-20,-20,
	20,1,1,1,1,1,1,20,
	20,1,1,1,1,1,1,20,
	20,1,1,1,1,1,1,20,
	20,1,1,1,1,1,1,20,
	-20,-20,1,1,1,1,-20,-20,
	99,-20,-20,20,20,-20,-20,99
};
//1:● -1:○ プレイヤー：1 ,AI:-1
int player = 1;


//盤面生成
void make_board() {
	board[3][3] = 1;
	board[4][4] = 1;
	board[3][4] = -1;
	board[4][3] = -1;
}


//盤面表示
void show_board() {
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
				cout << "●";
				wcount++;
			}
			else if (t == -1) {
				cout << "○";
				bcount++;
			}
		}
		cout << " ";
		rep(j, BOARD_SIZE) {
			printf("%2d",val[i][j]);
		}
		cout << endl;
	}
	cout << "先手(●):" << wcount << endl;
	cout << "後手(○):" << bcount << endl;
}

//手番表示
void show_player() {
	if (player == 1) {
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

bool check_puttable(int i, int j) {
	//盤面の範囲内かどうか
	if (!inside_board(i,j)) {
		return false;
	}

	//有効なマスか
	if (board[i][j] != 0) {
		return false;
	}

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
vector<pair<int, int>> get_puutable_places() {
	vector<pair<int, int>> ret;
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			if (check_puttable(i, j)) {
				ret.push_back({ i, j });
			}
		}
	}
	return ret;
}

//コマ設置
//simu:シミュレーションかどうか
//AIの手を考える時用
int place_stn(int i, int j, bool simu) {
	if (simu == true) {
		rep(i, BOARD_SIZE) {
			rep(j, BOARD_SIZE) {
				t_board[i][j] = board[i][j];
				
			}
		}
	}
	int count = 0;

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
					if (simu == false) {
						board[i + d_i * k][j + d_j * k] = player;
					}
					else {
						t_board[i + d_i * k][j + d_j * k] = player;
					}
					
					count++;
				}
				count--;
			}
		}
	}
	if (simu == false) board[i][j] = player;
	return count;
}

bool flag_fin() {
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			//どこかに置ける
			if (check_puttable(i, j)) {
				return true;
			}
		}
	}

	//プレイヤー交代で続行可能か
	player *= -1;
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			//どこかに置ける
			if (check_puttable(i, j)) {
				cout << "パスしました" << endl;
				player *= -1;
				return true;
			}
		}
	}
	cout << "どこにも置けません" << endl;
	if (player == 1)cout << "後手の勝利です" << endl;
	else if (player == -1)cout << "先手の勝利です" << endl;
	return false;
}

void judge() {
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

//AIの次の手を考える
void AIturn() {
	int count = 0;
	pair<int, int> point;
	int max_val = 0;
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			val[i][j] = 0;
			if (check_puttable(i, j)) {
				int a = place_stn(i, j, true);
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

pair<int, int> get_AI_hand() {
	for(auto hand:get_puutable_places()) dfs(0, true, hand);
}

void dfs(int depth, bool is_AI_turn, pair<int,int> hand) {

}

int evaluate(bool is_AI_turn, pair<int, int> hand) {
	int t_player = is_AI_turn ? -1 : 1;
	rep(i, BOARD_SIZE) {
		rep(j, BOARD_SIZE) {
			t_board[i][j] = board[i][j];
		}
	}
	int count = 0;
	int i = hand.first, j = hand.second;
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
				if (board[next_i][next_j] != t_player * -1) {
					break;
				}
				times++;
			}

			int next_i = i + d_i * times;
			int next_j = j + d_j * times;
			//注目するマスが有効範囲内か
			if (!inside_board(next_i, next_j))continue;
			//自分の駒で挟めていたら
			if (board[next_i][next_j] == t_player && times > 1) {
				rep(k, times) {
					//駒を裏返していく
					t_board[i + d_i * k][j + d_j * k] = t_player;
					count++;
				}
				count--;
			}
		}
	}
	board[i][j] = t_player;
	return count;
}


int main() {
	make_board();
	char a = 'a';
	int turn = 0;
	while (flag_fin()) {
		show_board();
		show_player();
		if (player == 1) {
			char tx, ty;
			int x, y;
			do {
				//x行目y列目
				//英字が行　数字が列
				cout << "配置する座標を入力してください" << endl;
				cin >> tx >> ty;
				input_check(&tx, &ty);
				//数値に変換
				x = tx - 'a';
				y = ty - '1';
			} while (!check_puttable(x, y));
			//cout << x << "行" << y << "列目" << endl
			//cout << "player:" << player << endl;
			place_stn(x, y, false);
		}
		else if (player == -1) {
			AIturn();
		}
		player *= -1;
		cout << "turn:" << ++turn << endl;
	}
	show_board();
	judge();
	return 0;
}


/*
3e
3d
3c
2d
5c
3b
1d
*/
