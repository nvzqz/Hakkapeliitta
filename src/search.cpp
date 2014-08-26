#include "search.hpp"
#include "eval.hpp"
#include "movegen.hpp"

const int Search::aspirationWindow = 50;
const int Search::nullReduction = 3;
const int Search::futilityDepth = 4;
const std::array<int, 1 + 4> Search::futilityMargins = {
    50, 125, 125, 300, 300
};
const int Search::lmrFullDepthMoves = 4;
const int Search::lmrReductionLimit = 3;

int Search::qSearch(Position & pos, int alpha, int beta)
{
    auto score = Evaluation::evaluate(pos);
    if (score > alpha)
    {
        if (score >= beta)
        {
            return score;
        }
        alpha = score;
    }
    else if (score + 1000 < alpha)
    {
        // If we are doing so badly that even capturing a queen for free won't help just return alpha.
        return alpha;
    }
    auto bestScore = score;
    auto delta = score + futilityMargins[0];

    std::vector<Move> moveStack;
    History history;
    MoveGen::generatePseudoLegalCaptureMoves(pos, moveStack);
    // Give moves a score here.
    for (auto & move : moveStack)
    {
        // Bad capture pruning + delta pruning. Assumes that the moves are sorted from highest SEE value to lowest.
        if (move.getScore() < 0 || (delta + move.getScore() < alpha))
        {
            return bestScore;
        }

        if (!pos.makeMove(move, history))
        {
            continue;
        }

        score = -qSearch(pos, -beta, -alpha);
        pos.unmakeMove(move, history);

        if (score > bestScore)
        {
            bestScore = score;
            if (score > alpha)
            {
                if (score >= beta)
                {
                    return score;
                }
                alpha = score;
            }
        }
    }

    return bestScore;
}



