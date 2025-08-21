#include "uci.hpp"
#include <print>

void uci_demo() {
    try {
    uci_interface uci;
    // std::stringstream ss;
    // ss << "uci\n";
    // ss << "setoption name Hash value 16\n";
    // ss << "isready\n";
    // // ss << "position fen 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1\n";
    // // ss << "position fen 1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - -\n"; // Qd1+
    // // ss << "position fen rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq -\n"; // e6
    // // ss << "position fen r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w - -\n"; // Nd5 
    // // ss << "position fen 1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w - -\n"; // Nf6
    // // ss << "position fen 4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - -\n";
    // // ss << "position fen 3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b - -\n";
    // // ss << "position fen r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b - -\n";
    // // ss << "position fen r2q1rk1/4bppp/p2p4/2pP4/3pP3/3Q4/PP1B1PPP/R3R1K1 w - -\n";
    // // ss << "position fen 2rq1rk1/1b2bp2/p3pn1p/np1p2p1/3P4/PPNQB1P1/3NPPBP/R1R3K1 b - - 9 20\n";
    // ss << "position startpos moves e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 a7a6 c1e3 e7e6 f2f3 b7b5 g2g4 h7h6 d1d2 b8d7 e1c1 c8b7 a2a3\n";
    // // ss << "position startpos\n";
    // ss << "go wtime 300000 btime 300000 winc 1000 binc 1000 movestogo 40\n";
    // // ss << "position startpos moves e2e4 e7e5 g1f3 b8c6 f1c4 f8c5 e1g1 g8f6 d2d3 e8g8 c2c3 d7d6 h2h3 h7h6 f1e1 a7a6 b1d2 c6a5 b2b4 a5c4 b4c5 c4a5 a2a4 f8e8 c5d6 c7d6 c3c4 f6d7 d2f1 d7c5 c1e3 a5c6 f1g3 d8a5 f3h4 c6b4 h4f5 b4d3 d1g4 g7g6 e1d1 a5d8 e3h6 b7b6 a1a3 d3f4 h6f4 e5f4 g4f4 c8f5 e4f5 g6g5 f4g4 f7f6 h3h4 a8a7 f2f4 a7h7 f4g5 f6g5 h4g5 h7g7 g5g6 g7d7 a3f3 e8e5 f5f6 c5e6 g3f5 d8f6 f5h6 g8g7 f3f6 g7f6 h6f7 e5f5 g6g7 e6g7 f7h6 d6d5 h6f5 g7f5 d1f1 d7f7 g4f5 f6e7 f5f7 e7d6 f7d5 d6c7\n";
    // // ss << "go wtime 335600 btime 276834 winc 1000 binc 1000 movestogo 35\n";
    // // ss << "position startpos moves e2e4 e7e5 g1f3 b8c6 d2d4 e5d4 f3d4 g8f6 b1c3 f8b4 d4c6 b7c6 f1d3 e8g8 e1g1 f8e8 c1g5 a8b8 f2f4 b4e7 e4e5 f6d5 d1h5 g7g6 h5h6 d5c3 b2c3 e7g5 f4g5 e8e5 f1f7 g8f7 h6h7 f7e6 h7g6 e6d5 g6f7 d5d6 h2h4 c6c5 f7f4 d8h8 g5g6 d6c6 a1f1 a7a6 f4g3 e5e7 g3g5 h8g7 h4h5 e7e8 h5h6 g7c3 g6g7 c3d4 g1h2 d7d6 d3g6 e8g8 g6f7 c8d7 f7g8 b8g8 g5f4 g8g7 f4d4 c5d4 h6g7 d7e6 a2a3 c6d7 f1f6 e6d5 f6f5 d5e6 f5f6 e6d5 f6f5 d5e6\n";
    // // ss << "go wtime 11597800 btime 117601 winc 0 binc 0 movestogo 1\n";
    // // ss << "stop\n";
    // ss << "quit\n";
    // uci.run(ss);
    uci.run(std::cin);
    } catch (const std::exception& e) {
        std::println("error: {}", e.what());
    }
}

int main() {
    uci_demo();
}
