#include "eval.hpp"
#include "hash.hpp"
#include "magic.hpp"
#include "ttable.hpp"

int drawScore = 0;

array<int, Squares> pieceSquareTableOpening[12];
array<int, Squares> pieceSquareTableEnding[12];

map<uint64_t, int> knownEndgames;

const std::array<int, 8> passedBonusOpening = {
    0, 0, 5, 10, 20, 25, 50, 0
};

const std::array<int, 8> passedBonusEnding = {
    0, 0, 10, 15, 30, 40, 80, 0
};

const std::array<int, 8> doubledPenaltyOpening = {
    8, 12, 16, 16, 16, 16, 12, 8
};

const std::array<int, 8> doubledPenaltyEnding = {
    16, 20, 24, 24, 24, 24, 20, 16
};

const std::array<int, 8> isolatedPenaltyOpening = {
    8, 12, 16, 16, 16, 16, 12, 8
};

const std::array<int, 8> isolatedPenaltyEnding = {
    12, 16, 20, 20, 20, 20, 16, 12
};

const std::array<int, 8> backwardPenaltyOpening = {
    6, 10, 12, 12, 12, 12, 10, 6
};

const std::array<int, 8> backwardPenaltyEnding = {
    12, 14, 16, 16, 16, 16, 14, 12
};

void initializeKnownEndgames()
{
	// King vs king: draw
	uint64_t matHash = materialHash[WhiteKing][0] ^ materialHash[BlackKing][0];
	knownEndgames[matHash] = 0;

	// King and a minor piece vs king: draw
	for (int i = White; i <= Black; i++)
	{
		for (int j = Knight; j <= Bishop; j++)
		{
			knownEndgames[matHash ^ materialHash[j + i * 6][0]] = 0;
		}
	}

	// King and two knights vs king: draw
	for (int i = White; i <= Black; i++)
	{
		knownEndgames[matHash ^ materialHash[Knight + i * 6][0] ^ materialHash[Knight + i * 6][1]] = 0;
	}

	// King and a minor piece vs king and a minor piece: draw
	for (int i = Knight; i <= Bishop; i++)
	{
		for (int j = Knight; j <= Bishop; j++)
		{
			knownEndgames[matHash ^ materialHash[White + i][0] ^ materialHash[Black * 6 + j][0]] = 0;
		}
	}

	// King and two bishops vs king and a bishop: draw
	for (int i = White; i <= Black; i++)
	{
		knownEndgames[matHash ^ materialHash[Bishop + i * 6][0] ^ materialHash[Bishop + i * 6][1] ^ materialHash[Bishop + !i * 6][0]] = 0;
	}

	// King and either two knights or a knight and a bishop vs king and a minor piece: draw
	for (int i = White; i <= Black; i++)
	{
		for (int j = Knight; j <= Bishop; j++)
		{
			for (int k = Knight; k <= Bishop; k++)
			{
				knownEndgames[matHash ^ materialHash[Knight + i * 6][0] ^ materialHash[j + i * 6][j == Knight] ^ materialHash[k + !i * 6][0]] = 0;
			}
		}
	}
}

void initializeEval()
{
	initializeKnownEndgames();

	array<int, Squares> flip = {
		56, 57, 58, 59, 60, 61, 62, 63,
		48, 49, 50, 51, 52, 53, 54, 55,
		40, 41, 42, 43, 44, 45, 46, 47,
		32, 33, 34, 35, 36, 37, 38, 39,
		24, 25, 26, 27, 28, 29, 30, 31,
		16, 17, 18, 19, 20, 21, 22, 23,
		8, 9, 10, 11, 12, 13, 14, 15,
		0, 1, 2, 3, 4, 5, 6, 7
	};

	for (int i = Pawn; i <= King; i++)
	{
		for (int sq = A1; sq <= H8; sq++)
		{
			pieceSquareTableOpening[i][sq] = openingPST[i][sq] + pieceValuesOpening[i];
			pieceSquareTableEnding[i][sq] = endingPST[i][sq] + pieceValuesEnding[i];

			pieceSquareTableOpening[i + Black * 6][sq] = -(openingPST[i][flip[sq]] + pieceValuesOpening[i]);
			pieceSquareTableEnding[i + Black * 6][sq] = -(endingPST[i][flip[sq]] + pieceValuesEnding[i]);
		}
	}
}

int mobilityEval(Position & pos, int phase, int & kingSafetyScore)
{
	int scoreOp = 0;
	int scoreEd = 0;
	int from, count;
	uint64_t occupied = pos.getOccupiedSquares();
	uint64_t tempPiece, tempMove;
    auto attackUnits = 0;

	// White
	uint64_t targetBitboard = ~pos.getPieces(White);
	auto opponentKingZone = kingZone[Black][bitScanForward(pos.getBitboard(Black, King))];

	tempPiece = pos.getBitboard(White, Knight);
	while (tempPiece)
	{
		from = bitScanForward(tempPiece);
		tempPiece &= (tempPiece - 1);

		tempMove = knightAttacks[from] & targetBitboard;

		count = popcnt(tempMove);
		scoreOp += mobilityOpening[Knight][count];
		scoreEd += mobilityEnding[Knight][count];

        attackUnits += attackWeight[Knight] * popcnt(tempMove & opponentKingZone);
	}

	tempPiece = pos.getBitboard(White, Bishop);
	while (tempPiece)
	{
		from = bitScanForward(tempPiece);
		tempPiece &= (tempPiece - 1);

		tempMove = bishopAttacks(from, occupied) & targetBitboard;

		count = popcnt(tempMove);
		scoreOp += mobilityOpening[Bishop][count];
		scoreEd += mobilityEnding[Bishop][count];

        tempMove = bishopAttacks(from, occupied ^ pos.getBitboard(White, Queen)) & targetBitboard;
        attackUnits += attackWeight[Bishop] * popcnt(tempMove & opponentKingZone);
	}

	tempPiece = pos.getBitboard(White, Rook);
	while (tempPiece)
	{
		from = bitScanForward(tempPiece);
		tempPiece &= (tempPiece - 1);

		tempMove = rookAttacks(from, occupied) & targetBitboard;

		count = popcnt(tempMove);
		scoreOp += mobilityOpening[Rook][count];
		scoreEd += mobilityEnding[Rook][count];

        tempMove = rookAttacks(from, occupied ^ pos.getBitboard(White, Queen) ^ pos.getBitboard(White, Rook)) & targetBitboard;
        attackUnits += attackWeight[Rook] * popcnt(tempMove & opponentKingZone);

		if (!(files[File(from)] & pos.getBitboard(White, Pawn)))
		{
			if (!(files[File(from)] & pos.getBitboard(Black, Pawn)))
			{
				scoreOp += rookOnOpenFileBonus;
			}
			else
			{
				scoreOp += rookOnSemiOpenFileBonus;
			}
		}
	}

	tempPiece = pos.getBitboard(White, Queen);
	while (tempPiece)
	{
		from = bitScanForward(tempPiece);
		tempPiece &= (tempPiece - 1);

		tempMove = queenAttacks(from, occupied) & targetBitboard;

		count = popcnt(tempMove);
		scoreOp += mobilityOpening[Queen][count];
		scoreEd += mobilityEnding[Queen][count];

        attackUnits += attackWeight[Queen] * popcnt(tempMove & opponentKingZone);
	}

    kingSafetyScore = kingSafetyTable[attackUnits];

	// Black
    opponentKingZone = kingZone[White][bitScanForward(pos.getBitboard(White, King))];
	targetBitboard = ~pos.getPieces(Black);
    attackUnits = 0;

	tempPiece = pos.getBitboard(Black, Knight);
	while (tempPiece)
	{
		from = bitScanForward(tempPiece);
		tempPiece &= (tempPiece - 1);

		tempMove = knightAttacks[from] & targetBitboard;

		count = popcnt(tempMove);
		scoreOp -= mobilityOpening[Knight][count];
		scoreEd -= mobilityEnding[Knight][count];

        attackUnits += attackWeight[Knight] * popcnt(tempMove & opponentKingZone);
	}

	tempPiece = pos.getBitboard(Black, Bishop);
	while (tempPiece)
	{
		from = bitScanForward(tempPiece);
		tempPiece &= (tempPiece - 1);

		tempMove = bishopAttacks(from, occupied) & targetBitboard;

		count = popcnt(tempMove);
		scoreOp -= mobilityOpening[Bishop][count];
		scoreEd -= mobilityEnding[Bishop][count];

        tempMove = bishopAttacks(from, occupied ^ pos.getBitboard(Black, Queen)) & targetBitboard;
        attackUnits += attackWeight[Bishop] * popcnt(tempMove & opponentKingZone);
	}

	tempPiece = pos.getBitboard(Black, Rook);
	while (tempPiece)
	{
		from = bitScanForward(tempPiece);
		tempPiece &= (tempPiece - 1);

		tempMove = rookAttacks(from, occupied) & targetBitboard;

		count = popcnt(tempMove);
		scoreOp -= mobilityOpening[Rook][count];
		scoreEd -= mobilityEnding[Rook][count];

        tempMove = rookAttacks(from, occupied ^ pos.getBitboard(Black, Queen) ^ pos.getBitboard(Black, Rook)) & targetBitboard;
        attackUnits += attackWeight[Rook] * popcnt(tempMove & opponentKingZone);

		if (!(files[File(from)] & pos.getBitboard(Black, Pawn)))
		{
			if (!(files[File(from)] & pos.getBitboard(White, Pawn)))
			{
				scoreOp -= rookOnOpenFileBonus;
			}
			else
			{
				scoreOp -= rookOnSemiOpenFileBonus;
			}
		}
	}

	tempPiece = pos.getBitboard(Black, Queen);
	while (tempPiece)
	{
		from = bitScanForward(tempPiece);
		tempPiece &= (tempPiece - 1);

		tempMove = queenAttacks(from, occupied) & targetBitboard;

		count = popcnt(tempMove);
		scoreOp -= mobilityOpening[Queen][count];
		scoreEd -= mobilityEnding[Queen][count];

        attackUnits += attackWeight[Queen] * popcnt(tempMove & opponentKingZone);
	}

    kingSafetyScore -= kingSafetyTable[attackUnits];

	return ((scoreOp * (256 - phase)) + (scoreEd * phase)) / 256;
}

int pawnStructureEval(Position & pos, int phase)
{
    auto scoreOp = 0, scoreEd = 0;
    int score;

    if (pttProbe(pos, scoreOp, scoreEd))
    {
        return ((scoreOp * (256 - phase)) + (scoreEd * phase)) / 256;
    }

    for (int c = White; c <= Black; ++c)
    {
        auto scoreOpForColor = 0, scoreEdForColor = 0;
        auto ownPawns = pos.getBitboard(!!c, Pawn);
        auto tempPawns = ownPawns;
        auto opponentPawns = pos.getBitboard(!c, Pawn);

        while (tempPawns)
        {
            auto from = bitScanForward(tempPawns);
            tempPawns &= (tempPawns - 1);
            auto pawnFile = File(from);
            auto pawnRank = (c ? 7 - Rank(from) : Rank(from));

            auto passedPawn = !(opponentPawns & passed[c][from]);
            auto doubledPawn = (ownPawns & (c ? rays[1][from] : rays[6][from])) != 0;
            auto isolatedPawn = !(ownPawns & isolated[from]);
            // 1. The pawn must be able to move forward.
            // 2. The stop square must be controlled by an enemy pawn.
            // 3. There musn't be any own pawns capable of defending the pawn. 
            // TODO: Check that this is correct.
            // TODO: test is the empty condition helping?
            auto backwardPawn = (pawnAttacks[c][from + 8 - 16 * c] & opponentPawns)
                //  && ((pos.getPiece(from + 8 - 16 * c) == Empty)
                && !(ownPawns & backward[c][from]);

            if (passedPawn)
            {
                scoreOpForColor += passedBonusOpening[pawnRank];
                scoreEdForColor += passedBonusEnding[pawnRank];
            }

            if (doubledPawn)
            {
                scoreOpForColor -= doubledPenaltyOpening[pawnFile];
                scoreEdForColor -= doubledPenaltyEnding[pawnFile];
            }

            if (isolatedPawn)
            {
                scoreOpForColor -= isolatedPenaltyOpening[pawnFile];
                scoreEdForColor -= isolatedPenaltyEnding[pawnFile];
            }

            if (backwardPawn)
            {
                scoreOpForColor -= backwardPenaltyOpening[pawnFile];
                scoreEdForColor -= backwardPenaltyEnding[pawnFile];
            }
        }

        scoreOp += (c == Black ? -scoreOpForColor : scoreOpForColor);
        scoreEd += (c == Black ? -scoreEdForColor : scoreEdForColor);
    }

    score = ((scoreOp * (256 - phase)) + (scoreEd * phase)) / 256;
    pttSave(pos, scoreOp, scoreEd);

    return score;
}


int kingSafetyEval(Position & pos, int phase, int score)
{
	int zone1, zone2;

	// White
	if (pos.getBitboard(White, King) & kingSide)
	{
		// Penalize pawns which have moved more than one square.
		zone1 = popcnt(0x00E0E0E0E0000000 & pos.getBitboard(White, Pawn));
		score -= pawnShelterAdvancedPawnPenalty * zone1;
		// A moved f-pawn isn't that severe, compensate.
		if (pos.getBitboard(White, Pawn) & rays[N][21])
		{
			score += (pawnShelterAdvancedPawnPenalty / 2);
		}

		// Penalize missing pawns from our pawn shelter.
		// Penalize missing opponent pawns as they allow the opponent to use his semi-open/open files to attack us.
		for (int i = 5; i < 8; i++)
		{
			if (!(files[i] & pos.getBitboard(White, Pawn)))
			{
				score -= pawnShelterMissingPawnPenalty;
				if (i == 5)
				{
					score += (pawnShelterMissingPawnPenalty / 2);
				}
			}
			if (!(files[i] & pos.getBitboard(Black, Pawn)))
			{
				score -= pawnShelterMissingOpponentPawnPenalty;
			}
		}

		// Pawn storm evaluation.
		// Penalize pawns on the 6th rank(from black's point of view).
		zone1 = popcnt(0x0000000000E00000 & pos.getBitboard(Black, Pawn));
		score -= pawnStormClosePenalty * zone1;

		// Penalize pawns on the 5th rank(from black's point of view).
		zone2 = popcnt(0x00000000E0000000 & pos.getBitboard(Black, Pawn));
		score -= pawnStormFarPenalty * zone2;
	}
	else if (pos.getBitboard(White, King) & queenSide)
	{
		zone1 = popcnt(0x0007070707000000 & pos.getBitboard(White, Pawn));
		score -= pawnShelterAdvancedPawnPenalty * zone1;
		if (pos.getBitboard(White, Pawn) & rays[N][18])
		{
			score += (pawnShelterAdvancedPawnPenalty / 2);
		}

		for (int i = 0; i < 3; i++)
		{
			if (!(files[i] & pos.getBitboard(White, Pawn)))
			{
				score -= pawnShelterMissingPawnPenalty;
				if (i == 2)
				{
					score += (pawnShelterMissingPawnPenalty / 2);
				}
			}
			if (!(files[i] & pos.getBitboard(Black, Pawn)))
			{
				score -= pawnShelterMissingOpponentPawnPenalty;
			}
		}

		zone1 = popcnt(0x0000000000070000 & pos.getBitboard(Black, Pawn));
		score -= pawnStormClosePenalty * zone1;

		zone2 = popcnt(0x0000000007000000 & pos.getBitboard(Black, Pawn));
		score -= pawnStormFarPenalty * zone2;
	}
	else 
	{
		// Penalize open files near the king.
		int kingFile = File(bitScanForward(pos.getBitboard(White, King)));
		for (int i = -1; i <= 1; i++)
		{
			if (!(files[kingFile + i] & pos.getBitboard(White, Pawn) & pos.getBitboard(Black, Pawn)))
			{
				score -= kingInCenterOpenFilePenalty;
			}
		}
	}

	// Black
	if (pos.getBitboard(Black, King) & kingSide)
	{
		zone1 = popcnt(0x000000E0E0E0E000 & pos.getBitboard(Black, Pawn));
		score += pawnShelterAdvancedPawnPenalty * zone1;
		if (pos.getBitboard(Black, Pawn) & rays[S][45])
		{
			score -= (pawnShelterAdvancedPawnPenalty / 2);
		}

		for (int i = 5; i < 8; i++)
		{
			if (!(files[i] & pos.getBitboard(Black, Pawn)))
			{
				score += pawnShelterMissingPawnPenalty;
				if (i == 5)
				{
					score -= (pawnShelterMissingPawnPenalty / 2);
				}
			}
			if (!(files[i] & pos.getBitboard(White, Pawn)))
			{
				score += pawnShelterMissingOpponentPawnPenalty;
			}
		}

		zone1 = popcnt(0x0000E00000000000 & pos.getBitboard(White, Pawn));
		score += pawnStormClosePenalty * zone1;

		zone2 = popcnt(0x000000E000000000 & pos.getBitboard(White, Pawn));
		score += pawnStormFarPenalty * zone2;
	}
	else if (pos.getBitboard(Black, King) & queenSide)
	{
		zone1 = popcnt(0x0000000707070700 & pos.getBitboard(Black, Pawn));
		score += pawnShelterAdvancedPawnPenalty * zone1;
		if (pos.getBitboard(Black, Pawn) & rays[S][42])
		{
			score -= (pawnShelterAdvancedPawnPenalty / 2);
		}

		for (int i = 0; i < 3; i++)
		{
			if (!(files[i] & pos.getBitboard(Black, Pawn)))
			{
				score += pawnShelterMissingPawnPenalty;
				if (i == 2)
				{
					score -= (pawnShelterMissingPawnPenalty / 2);
				}
			}
			if (!(files[i] & pos.getBitboard(White, Pawn)))
			{
				score += pawnShelterMissingOpponentPawnPenalty;
			}
		}

		zone1 = popcnt(0x0000070000000000 & pos.getBitboard(White, Pawn));
		score += pawnStormClosePenalty * zone1;

		zone2 = popcnt(0x0000000700000000 & pos.getBitboard(White, Pawn));
		score += pawnStormFarPenalty * zone2;
	}
	else
	{
		int kingFile = File(bitScanForward(pos.getBitboard(Black, King)));
		for (int i = -1; i <= 1; i++)
		{
			if (!(files[kingFile + i] & pos.getBitboard(White, Pawn) & pos.getBitboard(Black, Pawn)))
			{
				score += kingInCenterOpenFilePenalty;
			}
		}
	}

	return ((score * (256 - phase)) / 256);
}

int eval(Position & pos)
{
	int score, kingTropismScore = 0;
	int phase = pos.calculateGamePhase();

	// Checks if we are in a known endgame.
	// If we are we can straight away return the score for the endgame.
	// At the moment only detects draws, if wins will be included this must be made to return things in negamax fashion.
	if (knownEndgames.count(pos.getMaterialHash()))
	{
		return knownEndgames[pos.getMaterialHash()];
	}

	// Material + Piece-Square Tables
	score = ((pos.getScoreOp() * (256 - phase)) + (pos.getScoreEd() * phase)) / 256;

	if (popcnt(pos.getBitboard(White, Bishop)) == 2)
	{
		score += ((bishopPairBonusOpening * (256 - phase)) + (bishopPairBonusEnding * phase)) / 256;
	}
	if (popcnt(pos.getBitboard(Black, Bishop)) == 2)
	{
		score -= ((bishopPairBonusOpening * (256 - phase)) + (bishopPairBonusEnding * phase)) / 256;
	}

	// Mobility
	score += mobilityEval(pos, phase, kingTropismScore);

	// Pawn structure
	score += pawnStructureEval(pos, phase);

	// King safety
	score += kingSafetyEval(pos, phase, kingTropismScore);

    score += (pos.getSideToMove() ? -5 : 5);

    return (pos.getSideToMove() ? -score : score);
}