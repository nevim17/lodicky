#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <random>
#include <limits>

using namespace std;

enum Cell { EMPTY, SHIP, HIT, MISS };
enum Orientation { HORIZONTAL, VERTICAL };


struct Coord {
    int r, c;
    Coord(int rr = 0, int cc = 0) : r(rr), c(cc) {}
    bool operator==(const Coord& o) const { return r == o.r && c == o.c; }
};

struct Ship {
    string name;
    int length;
    vector<Coord> coords;
    vector<bool> hit;

    Ship(string n = "", int l = 0) : name(n), length(l), hit(l, false) {}

    bool is_sunk() const {
        return all_of(hit.begin(), hit.end(), [](bool h) { return h; });
    }

    bool occupies(const Coord& p, int& idx) const {
        for (size_t i = 0; i < coords.size(); i++)
            if (coords[i] == p) return idx = i, true;
        return false;
    }
};


struct Board {
    static const int R = 10, C = 10;

    Cell grid[R][C];
    vector<Ship> ships;

    Board() { clear(); }

    void clear() {
        for (auto& row : grid)
            for (auto& cell : row)
                cell = EMPTY;
        ships.clear();
    }

    bool in_bounds(const Coord& p) const {
        return p.r >= 0 && p.r < R && p.c >= 0 && p.c < C;
    }

    bool can_place(int r, int c, Orientation o, int len) const {
        for (int i = 0; i < len; i++) {
            int rr = r + (o == VERTICAL ? i : 0);
            int cc = c + (o == HORIZONTAL ? i : 0);
            if (!in_bounds({ rr, cc }) || grid[rr][cc] == SHIP)
                return false;
        }
        return true;
    }

    bool place_ship(int r, int c, Orientation o, Ship s) {
        if (!can_place(r, c, o, s.length)) return false;

        for (int i = 0; i < s.length; i++) {
            int rr = r + (o == VERTICAL ? i : 0);
            int cc = c + (o == HORIZONTAL ? i : 0);
            grid[rr][cc] = SHIP;
            s.coords.push_back({ rr, cc });
        }
        ships.push_back(s);
        return true;
    }

    pair<bool, string> shoot_at(Coord p) {
        if (!in_bounds(p)) return { false, "Shot outside board." };

        Cell& c = grid[p.r][p.c];

        if (c == HIT || c == MISS)
            return { false, "Already shot here." };

        if (c == SHIP) {
            c = HIT;
            for (auto& s : ships) {
                int idx;
                if (s.occupies(p, idx)) {
                    s.hit[idx] = true;
                    return { true, s.is_sunk() ? "Hit! You sank: " + s.name : "Hit!" };
                }
            }
        }

        c = MISS;
        return { false, "Miss." };
    }

    bool all_sunk() const {
        return all_of(ships.begin(), ships.end(),
            [](const Ship& s) { return s.is_sunk(); });
    }

    void display_row_labels() const {
        cout << "   ";
        for (int c = 0; c < C; c++) cout << char('A' + c) << ' ';
        cout << "\n";
    }

    void display(bool showShips = true) const {
        display_row_labels();
        for (int r = 0; r < R; r++) {
            cout << setw(2) << r + 1 << ' ';
            for (int c = 0; c < C; c++) {
                char ch = '.';
                switch (grid[r][c]) {
                case SHIP: ch = showShips ? 'S' : '.'; break;
                case HIT:  ch = 'X'; break;
                case MISS: ch = 'O'; break;
                default:   break;
                }
                cout << ch << ' ';
            }
            cout << "\n";
        }
    }
};


struct Player {
    string name;
    Board board;
    int shots_taken = 0;
    Player(string n = "Player") : name(n) {}
};


class Game {
public:
    Player p1{ "Player 1" }, p2{ "Player 2" };
    vector<pair<string, int>> ship_defs{
        {"Carrier",5}, {"Battleship",4}, {"Cruiser",3}, {"Submarine",3}, {"Destroyer",2}
    };

    void main_menu() {
        cout << "=== Battleship ===\n"
            << "1) New game (manual)\n"
            << "2) New game (random)\n"
            << "0) Exit\n"
            << "Choice: ";

        int opt;
        if (!(cin >> opt)) return;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (opt == 0) exit(0);

        setup_players(opt == 2);
        run();
    }

private:

  
    static bool parse_coord(string s, Coord& out) {
        s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());
        if (s.size() < 2) return false;

        char col = toupper(s[0]);
        if (col < 'A' || col > 'J') return false;

        string row = s.substr(1);
        if (!all_of(row.begin(), row.end(), ::isdigit)) return false;

        int r = stoi(row);
        if (r < 1 || r > 10) return false;

        out = { r - 1, col - 'A' };
        return true;
    }

    void pause_clear() {
        cout << "Press Enter...";
        cin.get();
        cout << string(40, '\n');
    }

 
    void setup_players(bool randomize) {
        p1 = Player("Player 1");
        p2 = Player("Player 2");

        cout << "\nSetup Player 1\n";
        randomize ? random_place_all(p1) : manual_place_all(p1);
        pause_clear();

        cout << "\nSetup Player 2\n";
        randomize ? random_place_all(p2) : manual_place_all(p2);
        pause_clear();
    }

    void manual_place_all(Player& pl) {
        cout << pl.name << " manual placement.\n";

        for (auto& sd : ship_defs) {
            bool placed = false;

            while (!placed) {
                pl.board.display(true);
                cout << "Place " << sd.first << " (size " << sd.second << ")\n";

                string coordStr;
                cout << "Coordinate (A5): ";
                getline(cin, coordStr);

                Coord p;
                if (!parse_coord(coordStr, p)) {
                    cout << "Invalid coordinate.\n";
                    continue;
                }

                cout << "Orientation (H/V): ";
                string o;
                getline(cin, o);
                if (o.empty()) continue;

                Orientation ORI =
                    (toupper(o[0]) == 'H') ? HORIZONTAL :
                    (toupper(o[0]) == 'V') ? VERTICAL :
                    Orientation(-1);

                if (ORI == Orientation(-1)) {
                    cout << "Invalid orientation.\n";
                    continue;
                }

                Ship s(sd.first, sd.second);
                placed = pl.board.place_ship(p.r, p.c, ORI, s);
                if (!placed) cout << "Invalid placement.\n";
            }

            pause_clear();
        }
    }

    void random_place_all(Player& pl) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dr(0, Board::R - 1);
        uniform_int_distribution<> dc(0, Board::C - 1);
        uniform_int_distribution<> dor(0, 1);

        for (auto& sd : ship_defs) {
            bool placed = false;

            for (int t = 0; t < 500 && !placed; t++) {
                int r = dr(gen), c = dc(gen);
                Orientation o = dor(gen) ? HORIZONTAL : VERTICAL;

                Ship s(sd.first, sd.second);
                placed = pl.board.place_ship(r, c, o, s);
            }
        }

        cout << pl.name << " ships placed randomly.\n";
    }


    void run() {
        Player* cur = &p1;
        Player* opp = &p2;

        while (true) {
            cout << "\n--- " << cur->name << " ---\n";
            cout << "Your board:\n";
            cur->board.display(true);

            cout << "\nOpponent view:\n";
            display_opponent_view(opp->board);

            cout << "\nShot (A5), P=pause: ";
            string s;
            getline(cin, s);

            if (!s.empty() && toupper(s[0]) == 'P') {
                pause_clear();
                continue;
            }

            Coord p;
            if (!parse_coord(s, p)) {
                cout << "Invalid.\n";
                continue;
            }

            auto res = opp->board.shoot_at(p);

            if (res.second == "Already shot here.") {
                cout << res.second << "\n";
                continue;
            }

            cur->shots_taken++;
            cout << res.second << "\n";

            if (res.first) {
                // hit
                if (opp->board.all_sunk()) {
                    cout << "\n*** " << cur->name << " WINS! ***\n";
                    show_stats();
                    return;
                }
                cout << "Shoot again!\n";
            }
            else {
                swap(cur, opp);
            }
        }
    }

    void display_opponent_view(const Board& b) const {
        b.display_row_labels();
        for (int r = 0; r < Board::R; r++) {
            cout << setw(2) << r + 1 << ' ';
            for (int c = 0; c < Board::C; c++) {
                Cell cell = b.grid[r][c];
                cout << (cell == HIT ? 'X' : cell == MISS ? 'O' : '.') << ' ';
            }
            cout << '\n';
        }
    }

    void show_stats() {
        cout << "\n=== Stats ===\n";
        cout << p1.name << ": " << p1.shots_taken << " shots\n";
        cout << p2.name << ": " << p2.shots_taken << " shots\n";
    }
};


int main() {
    Game g;
    while (true) g.main_menu();
    return 0;
}