/*

HOW TO USE THE DATA FILE TO EVALUATE 7-, 8- AND 9-CARD OMAHA HANDS

# Evaluating a 9-card hand

	Assume that you are given a board: b0, b1, b2, b3, b4 and 
	a pocket: p0, p1, p2, p3. Denote the hand ranks array
	by HR.

	# Check for flushes and save the suit

		The flush suit can be found by indexing HR nine times,
		board first, then pocket, with initial offset of 106:

			board_offset := HR[HR[HR[HR[HR[
				106 + b0] + b1] + b2] + b3] + b4]
			flush_suit := HR[HR[HR[HR[board_offset + p0] + p1] + p2] + p3]

	# Evaluate hand ignoring all suits

		The base offset for non-flush hands is stored in HR[0],
		from there you add 53 and index nine times:

			board_offset := HR[HR[HR[HR[HR[
				HR[0] + 53 + b0] + b1] + b2] + b3] + b4]
			score := HR[HR[HR[HR[board_offset + p0] + p1] + p2] + p3]

	# Only if flush_suit != 0, check if flush score is better than non-flush:

		The base offset for flushes is stored in HR[1], plus 56, however you 
		have to add (4 - flush_suit) to this pointer.

			HR_f = HR + (4 - flush_suit)
			board_offset := HR_f[HR_f[HR_f[HR_f[HR_f[
				HR[1] + 56 + b0] + b1] + b2] + b3] + b4]
			flush_score := HR[HR[HR[HR[board_offset + p0] + p1] + p2] + p3]
			if (flush_score > score)
				score := flush_score

		Alternatively, you can add (4 - flush_suit) manually every time you
		index, but it will just take longer.

# Evaluating 7- and 8-card hands

	Pass zero for the first board card to evaluate a 7-card hand, and pass
	two zeros for the first two board cards to evaluate a 8-card hand.

	This has to be done for all three offset blocks, i.e. for 7-card hand

	Summarizing,

		Offsets for a 9-card hand:
			flush suits (HR) 		106
			flush ranks (HR_f): 	HR[1] + 56
			non-flush ranks (HR): 	HR[0] + 53

		Offsets for a 8-card hand:
			flush suits (HR): 		HR[106]
			flush ranks (HR_f): 	HR_f[HR[1] + 56]
			non-flush ranks (HR): 	HR[HR[0] + 53]

		Offsets for a 8-card hand:
			flush suits (HR): 		HR[HR[106]]
			flush ranks (HR_f): 	HR_f[HR_f[HR[1] + 56]]
			non-flush ranks (HR): 	HR[HR[HR[0] + 53]]
*/

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <locale>
#include <string>
#include <tr1/unordered_map>
#include <exception>
#include <string.h>

#include "rayutils.h"
#include "arrays.h"

const int ANY_CARD = 1;			// placeholder for flush ranks eval
const int SKIP_BOARD = 53; 		// placeholder for 7-8 card hands

const int pocket_perms[6][2] = {
	{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}
};
const int board_perms[10][3] = {
	{0, 1, 2}, 															// 3-5 cards
	{0, 1, 3}, {0, 2, 3}, {1, 2, 3}, 									// 4-5 cards
	{0, 1, 4}, {0, 2, 4}, {0, 3, 4}, {1, 2, 4}, {1, 3, 4}, {2, 3, 4}	// 5 cards
}; 	
const int n_pocket_perms = 6;

void skip_board(int *board, int &n_board)
{
	int temp[5] = {0, 0, 0, 0, 0};
	n_board = 0;
	for (int i = 0; i < 5; i++)
		if (board[i] != SKIP_BOARD)
			temp[n_board++] = board[i];
	memcpy(board, temp, 5 * sizeof(int));
}

int count_cards(int64_t id)
{
	return (((id) & 0x7F) != 0) + 
		(((id >> 7) & 0x7F) != 0) + 
		(((id >> 14) & 0x7F) != 0) + 
		(((id >> 21) & 0x7F) != 0) + 
		(((id >> 28) & 0x7F) != 0) + 
		(((id >> 35) & 0x7F) != 0) + 
		(((id >> 42) & 0x7F) != 0) + 
		(((id >> 49) & 0x7F) != 0) + 
		(((id >> 56) & 0x7F) != 0);
}

void add_card(int new_card, int *pocket, int *board, int &n_pocket, int &n_board)
{
	if (n_board < 5)
		board[n_board++] = new_card;
	else
		pocket[n_pocket++] = new_card;
}

void unpack64(int64_t id, int *pocket, int *board, int &n_pocket, int &n_board)
{
	memset(pocket, 0, 4 * sizeof(int));
	memset(board, 0, 5 * sizeof(int));
	n_pocket = 0;
	n_board = 0;
	int card;
	for (int i = 0; i < 9; i++)
	{
		if ((card = (int) ((id >> (7 * i)) & 0x7F)) != 0)
		{
			if (i < 5)
				board[n_board++] = card;
			else
				pocket[n_pocket++] = card;
		}
	}
}

int64_t pack64(int *pocket, int *board)
{
	std::sort(pocket, pocket + 4);
	std::sort(board, board + 5);
	std::reverse(pocket, pocket + 4);
	std::reverse(board, board + 5);

	return ((int64_t) board[0] +
		 ((int64_t) board[1] << 7) +
		 ((int64_t) board[2] << 14) + 
		 ((int64_t) board[3] << 21) +
		 ((int64_t) board[4] << 28) +
		 ((int64_t) pocket[0] << 35) +
		 ((int64_t) pocket[1] << 42) +
		 ((int64_t) pocket[2] << 49) +
		 ((int64_t) pocket[3] << 56));    
}

void print_id(int64_t id, bool indent=false)
{
	int board[5], pocket[4], n_pocket, n_board;
	unpack64(id, pocket, board, n_pocket, n_board);
	std::cout << (indent ? "\t" : "") << "id:       " << id << "\n";
	std::cout << (indent ? "\t" : "") << "n_board:  " << n_board << "\n";
	std::cout << (indent ? "\t" : "") << "board:    [";
	for (int i = 0; i < n_board; i++)
		std::cout << board[i] << (((i + 1) == n_board) ? "" : ", ");
	std::cout << "]\n";
	std::cout << (indent ? "\t" : "") << "n_pocket:  " << n_pocket << "\n";
	std::cout << (indent ? "\t" : "") << "pocket:   [";
	for (int i = 0; i < n_pocket; i++)
		std::cout << pocket[i] << (((i + 1) == n_pocket) ? "" : ", ");
	std::cout << "]\n";
}

int64_t add_card_to_id_flush_suits(int64_t id, int new_card)
{
	int pocket[4], board[5], n_pocket, n_board;
	new_card = new_card == 0 ? SKIP_BOARD : ((new_card - 1) & 3) + 1;
	unpack64(id, pocket, board, n_pocket, n_board);
	add_card(new_card, pocket, board, n_pocket, n_board);
	return pack64(pocket, board);
}

int eval_flush_suits(int64_t id)
{
	int pocket[4], board[5], n_pocket, n_board,
		nsp[5] = {0, 0, 0, 0, 0}, nsb[5] = {0, 0, 0, 0, 0};
	unpack64(id, pocket, board, n_pocket, n_board);
	for (int i = 0; i < n_pocket; i++)
		nsp[pocket[i]] = MIN(nsp[pocket[i]] + 1, 2);
	for (int i = 0; i < n_board; i++)
		if (board[i] != SKIP_BOARD)
			nsb[board[i]] = MIN(nsb[board[i]] + 1, 3);
	for (int suit = 1; suit <= 4; suit++)
		if ((nsp[suit] + nsb[suit]) >= 5)
			return suit;
	return -1;
}

int64_t add_card_to_id_flush_ranks(int64_t id, int new_card, int flush_suit)
{
	int pocket[4], board[5], n_pocket, n_board, nsp = 0, nsb = 0, i;
	bool debug = false;

	// the rank is 2-14 if suited or 1 otherwise (for sorting purposes)
	if (new_card == 0)
		new_card = SKIP_BOARD;
	else
		new_card = ((((new_card - 1) & 3) + 1) == flush_suit) ? 
			(2 + ((new_card - 1) >> 2) & 0xF) : ANY_CARD;

	if (debug) std::cout << "old deck:\n";
	if (debug) print_id(id, true);
	if (debug) std::cout << "new_card: " << new_card << "\n";

	unpack64(id, pocket, board, n_pocket, n_board);

	for (i = 0; i < n_pocket; i++)
		if (pocket[i] != ANY_CARD && pocket[i] != SKIP_BOARD && pocket[i] == new_card)
		{
			if (debug) std::cout << "halting, duplicate pocket: " << pocket[i] << "\n";
			return 0;
		}
	for (i = 0; i < n_board; i++)
		if (board[i] != ANY_CARD && board[i] != SKIP_BOARD && board[i] == new_card)
		{
			if (debug) std::cout << "halting, duplicate board: " << board[i] << "\n";
			return 0;
		}

	if (debug) std::cout << "no duplicates, adding card...\n";

	add_card(new_card, pocket, board, n_pocket, n_board);

	for (i = 0; i < n_pocket; i++)
		if (pocket[i] != ANY_CARD && pocket[i] != 0) 
			nsp++;
	for (i = 0; i < n_board; i++)
		if (board[i] != ANY_CARD && board[i] != SKIP_BOARD && board[i] != 0)
			nsb++;

	if (debug) std::cout << "nsp = " << nsp << ", nsb = " << nsb << "\n";

	if (n_board == 4 && nsb <= 1)
		return 0;
	if (n_board == 5 && nsb <= 2)
		return 0;
	if (n_board == 5 && n_pocket == 3 && nsp == 0)
		return 0;
	if (n_board == 5 && n_pocket == 4 && nsp <= 1)
		return 0;

	if (debug) std::cout << "packing...\n";

	if (debug) std::cout << "new deck:\n";
	if (debug) print_id(pack64(pocket, board), true);

	return pack64(pocket, board);
}

int64_t add_card_to_id_flush_ranks_1(int64_t id, int new_card)
{
	return add_card_to_id_flush_ranks(id, new_card, 1);	
}

int64_t add_card_to_id_flush_ranks_2(int64_t id, int new_card)
{
	return add_card_to_id_flush_ranks(id, new_card, 2);	
}

int64_t add_card_to_id_flush_ranks_3(int64_t id, int new_card)
{
	return add_card_to_id_flush_ranks(id, new_card, 3);	
}

int64_t add_card_to_id_flush_ranks_4(int64_t id, int new_card)
{
	return add_card_to_id_flush_ranks(id, new_card, 4);	
}

int eval_flush_ranks(int64_t id)
{
	int pocket[4], board[5], n_pocket, n_board;
	unpack64(id, pocket, board, n_pocket, n_board);
	skip_board(board, n_board);
	if (!pocket[0] || !pocket[1] || !board[0] || !board[1] || !board[2])
	{	
		std::cout << "\neval_flush_ranks(): " << id << ": zero encountered.\n";
		return -1;
	}
	if (pocket[0] == ANY_CARD || pocket[1] == ANY_CARD || board[0] == ANY_CARD ||
		board[1] == ANY_CARD || board[2] == ANY_CARD)
		return -1;

	// sadly, we have to account for straight flushes...
	int n = n_pocket + n_board;
	const int pocket_perms[2][6] = {{0, 0, 0, 1, 1, 2}, {1, 2, 3, 2, 3, 3}};
	const int n_pocket_perms = 6;
	const int board_perms[10][3] = {
		{0, 1, 2}, // 3, 4, 5
		{0, 1, 3}, {0, 2, 3}, {1, 2, 3}, // 4, 5
		{0, 1, 4}, {0, 2, 4}, {0, 3, 4}, {1, 2, 4}, {1, 3, 4}, {2, 3, 4}}; // 5
	int n_board_perms = n == 9 ? 10 : n == 8 ? 4 : n == 7 ? 1 : -1;
	int np, nb, best = 8191, q = 0;
	for (np = 0; np < n_pocket_perms; np++)
	{
		for (nb = 0; nb < n_board_perms; nb++)
		{
			int rank1 = pocket[pocket_perms[0][np]] - 2,
				rank2 = pocket[pocket_perms[1][np]] - 2,
				rank3 = board[board_perms[nb][0]] - 2,
				rank4 = board[board_perms[nb][1]] - 2,
				rank5 = board[board_perms[nb][2]] - 2;
			if (rank1 < 0 || rank2 < 0 || rank3 < 0 || rank4 < 0 || rank5 < 0 ||
				rank1 > 12 || rank2 > 12 || rank3 > 12 || rank4 > 12 || rank5 > 12)
				continue;
			q = flushes[(1 << rank1) | (1 << rank2) | (1 << rank3) | (1 << rank4) | (1 << rank5)];
			if (q < best)
				best = q;
		}
	}
	return cactus_to_ray(best);
}

int64_t add_card_to_id_no_flush(int64_t id, int new_card)
{
	int pocket[4], board[5], n_pocket, n_board, i,
		n_rank[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	if (new_card == 0)
		new_card = SKIP_BOARD;
	else
		new_card = 1 + ((new_card - 1) >> 2) & 0xF;
	unpack64(id, pocket, board, n_pocket, n_board);
	for (i = 0; i < n_pocket; i++)
		n_rank[pocket[i]]++;
	for (i = 0; i < n_board; i++)
		if (board[i] != SKIP_BOARD)
			n_rank[board[i]]++;
	add_card(new_card, pocket, board, n_pocket, n_board);
	if (new_card != SKIP_BOARD)
		n_rank[new_card]++;
	for (i = 1; i <= 13; i++)
		if (n_rank[i] > 4)
			return 0;
	return pack64(pocket, board);
}

int card_to_cactus(int rank, int suit)
{
	static bool buffered = false;
	static int cards[14][5];
	const int primes[13] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41};
	if (!buffered)
	{
		for (int r = 0; r < 14; r++)
			cards[r][0] = 0;
		for (int s = 0; s < 5; s++)
			cards[0][s] = 0;
		for (int r = 1; r < 14; r++)
			for (int s = 1; s < 5; s++)
				cards[r][s] = primes[(r - 1)] | ((r - 1) << 8) | 
					(1 << (s + 11)) | (1 << (16 + (r - 1)));
		buffered = true;
	}
	return cards[rank][suit];
}

int eval_cactus_no_flush(int c1, int c2, int c3, int c4, int c5)
{
	int s = unique5[(c1 | c2 | c3 | c4 | c5) >> 16];
	if (s)
		return s;
	else
		return values[cactus_findit((c1 & 0xFF) * (c2 & 0xFF) * 
			(c3 & 0xFF) * (c4 & 0xFF) * (c5 & 0xFF))];
}

int eval_no_flush(int64_t id)
{

	int pocket[4], board[5], n_pocket, n_board, n, n_board_perms;
	unpack64(id, pocket, board, n_pocket, n_board);
	skip_board(board, n_board);
	n = n_pocket + n_board;
	n_board_perms = n == 9 ? 10 : n == 8 ? 4 : n == 7 ? 1 : -1;
	if (n_pocket < 4 || n_board < 3)
	{	
		std::cout << "\neval_no_flush() encountered invalid # of cards, shouldn't happen...\n";
		return 0;
	}

	// convert to cactus and add random suits
	int suit = 0;
	for (int i = 0; i < n_pocket; i++)
		pocket[i] = card_to_cactus(pocket[i], ((suit++) % 4) + 1);
	for (int i = 0; i < n_board; i++)
		board[i] = card_to_cactus(board[i], ((suit++) % 4) + 1);

	int np, nb, best = 8191, q = 0;
	for (np = 0; np < n_pocket_perms; np++)
	{
		for (nb = 0; nb < n_board_perms; nb++)
		{
			q = eval_cactus_no_flush(pocket[pocket_perms[np][0]],
				pocket[pocket_perms[np][1]],
				board[board_perms[nb][0]],
				board[board_perms[nb][1]],
				board[board_perms[nb][2]]);
			if (q < best)
				best = q;
		}
	}
	return cactus_to_ray(best);
}

/*
	Allow to "skip" a board card only when there are no cards on the
	table or just one skipped. A board card can be skipped by passing
	the offset of 0, however internally it will be stored as 53. 
	Skipping is only allowed for the first and second cards (essentially, 
	we just start off from a different offset).
*/

void generate_ids(size_t size, std::vector<int64_t> &id_list,
	int64_t (*add_card_to_id) (int64_t, int))

{
	std::vector<int64_t> id_queue_1, id_queue_2;
	int64_t new_id = 0;
	id_list.reserve(size);
	id_list.clear();
	id_list.push_back(0LL);
	id_queue_1.reserve(size);
	id_queue_2.reserve(size);
	id_queue_1.push_back(0LL);
	for (int n_cards = 1; n_cards <= 8; n_cards++)
	{
		std::cout << "\nGenerating " << n_cards << "-card IDs:\n";

		int n1 = (int) id_queue_1.size(); // LOL, if we don't do this, expect nasty bugs
		for (int i = 0; i < n1; i++)
		{
			std::cout << "\r\t" << "Processing ID " << i + 1 << " / " <<
				n1 << "...";

			int64_t id = id_queue_1[i];
			int min_card = (n_cards <= 2) ? 0 : 1; // board skipping
			for (int new_card = min_card; new_card <= 52; new_card++)
				if ((new_id = (*add_card_to_id)(id, new_card)) != 0)
					id_queue_2.push_back(new_id);
		}
		std::cout << "\n\t" << id_queue_1.size() << ", " << id_queue_2.size() << "\n";
		size_t size = id_queue_2.size();
		std::cout << "\n\tGenerated " << id_queue_2.size() << " IDs." <<
			"\n\tSorting and dropping duplicates...";
		std::sort(id_queue_2.begin(), id_queue_2.end());
		id_queue_2.erase(std::unique(id_queue_2.begin(), id_queue_2.end()), 
			id_queue_2.end());
		std::cout << " dropped " << (size - id_queue_2.size()) << " IDS.";
		std::cout << "\n\tInserting IDS into the final list...";
		id_list.insert(id_list.end(), id_queue_2.begin(), id_queue_2.end());
		std::cout << " total: " << id_list.size() << " IDS.";
		std::cout << "\n\tResetting the queue...";
		id_queue_1.swap(id_queue_2);
		id_queue_2.clear();
	}
	std::cout << "\n\tFinished: generated " << id_list.size() << " IDs, sorting....";
	std::sort(id_list.begin(), id_list.end());
	std::vector<int64_t>().swap(id_queue_1);
	std::vector<int64_t>().swap(id_queue_2);
}

/*

	GLOBAL

	0-53
		0: flush ranks base offset (add suit * 53 to get true offset)
		1: 

	FLUSH_RANKS
		0-53: 			reserved
		53*1 + 1-52:	suit 1 starting point
		53*2 + 1-52:	suit 2 starting point
		53*3 + 1-52:	suit 3 starting point
		53*4 + 1-52:	suit 4 starting point
		>= 53*5: 		normal layers

	FLUSH_RANKS SHIFTING

		Assume: there are only 2 ranks, 1-2 and 4 suits, 1-4
		Denote:  -- , [1], [2] ~ any card, rank 1, rank 2
		        [b], [d] ~ base offset, dummy slot

			      [b]  1(1) 1(2) 1(3) 1(4) 2(1) 2(2) 2(3) 2(4) [d]  [d]  [d]

		suit 1:        [1]   --  --   --   [2]  --   --   --   --   --   -- 
		suit 2:         --  [1]  --   --   --   [2]  --   --   --   --   -- 
		suit 4:         --   --  [1]  --   --   --   [2]  --   --   --   -- 
		suit 3:         --   --  --   [1]  --   --   --   [2]  --   --   -- 
*/

void process_ids(std::vector<int64_t> ids, int offset, int offset_value,
	std::vector<int> &hand_ranks, int64_t (*add_card_to_id)(int64_t, int),
	int (*eval_id)(int64_t), int n_dummy, int dummy_card,
	std::tr1::unordered_map<int, int> map=std::tr1::unordered_map<int, int>())
{
	// offset + 0: special value
	// offset + 1-52: loop back to offset + 0
	// offset + 53: starting point
	// offset + 54+: normal operation

	int n = (int) ids.size(), i, id_index, num_cards, new_card;
	int64_t id, new_id;
	int block_size = 53 + n_dummy;
	std::tr1::unordered_map<int64_t, int> hash_table;

	for (i = 0; i < n; i++)
	{
		std::cout << "\r\tCreating unordered map... " << (i + 1) << " / " << n;
		hash_table.insert(std::tr1::unordered_map<int64_t, int>::value_type(ids[i], i));
	}
	std::cout << "\n";

	hand_ranks[offset] = offset_value;
	for (i = 1; i <= 52; i++)
		hand_ranks[offset + i] = offset;
	for (i = 53; i < block_size; i++)
		hand_ranks[offset + i] = offset;
	for (i = 0; i < n; i++)
	{
		id = ids[i];
		std::cout << "\r\tProcessing ID " << i + 1 << " out of " << n << " (" << id << ")..." ;
		id_index = offset + block_size + i * block_size;
		num_cards = count_cards(id);
		hand_ranks[id_index] = offset; // safety backup

		int min_card = (num_cards <= 1) ? 0 : 1; // board skipping
		int dummy_value = -1;
		for (new_card = min_card; new_card <= 52; new_card++)
		{			
			new_id = (*add_card_to_id)(id, new_card);
			if (new_id && ((num_cards + 1) == 9))
			{
				int value = (*eval_id)(new_id);
				hand_ranks[id_index + new_card] = map.count(value) ? map[value] : value;
			}
			else if (new_id)
				hand_ranks[id_index + new_card] = offset + block_size + 
					hash_table[new_id] * block_size;
			else
				hand_ranks[id_index + new_card] = offset; // < 9 cards and id is not valid
			if (new_card == dummy_card)
				dummy_value = hand_ranks[id_index + new_card];
		}
		if (dummy_value != -1)		
			for (new_card = 53; new_card < block_size; new_card++)
				hand_ranks[id_index + new_card] = dummy_value;
	}

}

// int generate_handranks(int *hand_ranks)
int generate_handranks(std::vector<int> &hand_ranks)
{
	std::vector<int64_t> id_fs, id_fr1, id_fr2, id_fr3, id_fr4, id_nf;

	std::cout << "\n====== PHASE 1 (GENERATE IDS) ======";

	std::cout << "\n\n>> IDs for flush suits... \n";
	generate_ids(100e3, id_fs, add_card_to_id_flush_suits);

	std::cout << "\n\n>> IDs for flush ranks (suit #4)... \n";	
	generate_ids(10e6, id_fr4, add_card_to_id_flush_ranks_4);

	std::cout << "\n\n>> IDs for non-flush hands... \n";	
	generate_ids(100e6, id_nf, add_card_to_id_no_flush);

	int n_fs = (int) id_fs.size(), n_fr4 = (int) id_fr4.size(), 
		n_nf = (int) id_nf.size();

	std::cout << "\n\n\n====== PHASE 2 (PROCESS IDS) ======";

	int offset_fs = 53, offset_fr4, offset_nf, max_rank;
	offset_fr4 = offset_fs + 53 + n_fs * 53;
	offset_nf = offset_fr4 + (53 + 3) + n_fr4 * (53 + 3);
	max_rank = offset_nf + 53 + n_nf * 53;

	std::cout << "\n\nMAX_RANK = " << max_rank << "\n";
	std::vector<int>(max_rank, 0).swap(hand_ranks);

	hand_ranks[0] = offset_nf;
	hand_ranks[1] = offset_fr4;

	std::cout << "\n\nEvaluating flush suits...\n";
	std::tr1::unordered_map<int, int> map_fs;
	map_fs.insert(std::tr1::unordered_map<int, int>::value_type(-1, 0));
	process_ids(id_fs, offset_fs, offset_nf, hand_ranks,
		add_card_to_id_flush_suits, eval_flush_suits, 0, 0, map_fs);

	std::cout << "\n\nEvaluating flush ranks (suit #4 + dummies)...\n";
	std::tr1::unordered_map<int, int> map_fr4;
	map_fr4.insert(std::tr1::unordered_map<int, int>::value_type(-1, offset_fr4));
	process_ids(id_fr4, offset_fr4, 0, hand_ranks,
		add_card_to_id_flush_ranks_4, eval_flush_ranks, 3, 1, map_fr4);

	std::cout << "\n\nEvaluating non-flush hands...\n";
	std::tr1::unordered_map<int, int> map_nf;
	map_nf.insert(std::tr1::unordered_map<int, int>::value_type(-1, offset_nf));
	process_ids(id_nf, offset_nf, 0, hand_ranks,
		add_card_to_id_no_flush, eval_no_flush, 0, 0, map_nf);

	// hand_ranks.resize(max_rank);

	std::cout << "\n\nDone.\n";

	return max_rank;
}

int test_all_handranks(const char *filename, const char *filename7)
{
	int *HR_new = smart_load(filename);
	int *HR_old = smart_load(filename7);

	int c[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	int64_t n = 0;
	int64_t N[3] = {133784560LL, 752538150LL,
		3679075400LL}; // C(52, 7), C(52, 8), C(52, 9)
	int min0[3] = {0,  0,  1};
	int max0[3] = {0,  0,  52};
	int min1[3] = {0,  1,  1};
	int max1[3] = {0,  52, 52};
	int n_board_perms[3] = {1, 4, 10};
	int board_paths[10];

	// int no_flush_offset = HR_new[0],
	// 	flush_offset = HR_new[1];

	for (int k = 0; k < 3; k++)
	{
		std::cout << "\nChecking all " << (7 + k) << "-card sorted combinations...\n";
		n = 0;

		for (c[0] = min0[k]; c[0] <= max0[k]; c[0]++)
		{
			int fs0 = HR_new[106 + c[0]]; // flush suit
			int snf0 = HR_new[HR_new[0] + 53 + c[0]]; // score no flush
			for (c[1] = (min1[k] == 0) ? 0 : (c[0] + 1); c[1] <= max1[k]; c[1]++)
			{
				int fs1 = HR_new[fs0 + c[1]];
				int snf1 = HR_new[snf0 + c[1]];
				for (c[2] = c[1] + 1; c[2] <= 52; c[2]++)
				{
					int fs2 = HR_new[fs1 + c[2]];
					int snf2 = HR_new[snf1 + c[2]];
					for (c[3] = c[2] + 1; c[3] <= 52; c[3]++)
					{
						int fs3 = HR_new[fs2 + c[3]];
						int snf3 = HR_new[snf2 + c[3]];
						for (c[4] = c[3] + 1; c[4] <= 52; c[4]++)
						{
							int fs4 = HR_new[fs3 + c[4]];
							int snf4 = HR_new[snf3 + c[4]];

							for (int nb = 0; nb < n_board_perms[k]; nb++)
								board_paths[nb] = HR_old[HR_old[HR_old[53 + 
									c[(2 - k) + board_perms[nb][0]]] + 
									c[(2 - k) + board_perms[nb][1]]] +
									c[(2 - k) + board_perms[nb][2]]];

							for (c[5] = c[4] + 1; c[5] <= 52; c[5]++)
							{
								int fs5 = HR_new[fs4 + c[5]];
								int snf5 = HR_new[snf4 + c[5]];
								for (c[6] = c[5] + 1; c[6] <= 52; c[6]++)
								{
									int fs6 = HR_new[fs5 + c[6]];
									int snf6 = HR_new[snf5 + c[6]];
									for (c[7] = c[6] + 1; c[7] <= 52; c[7]++)
									{
										int fs7 = HR_new[fs6 + c[7]];
										int snf7 = HR_new[snf6 + c[7]];
										for (c[8] = c[7] + 1; c[8] <= 52; c[8]++)
										{
											int fs = HR_new[fs7 + c[8]],
												score_new = HR_new[snf7 + c[8]];
											if (fs != 0)
											{
												int *HR_flush = HR_new + (4 - fs);
												int score_flush = HR_new[1] + 56;
												for (int i = 0; i < 9; i++)
													score_flush = HR_flush[score_flush + c[i]];
												score_new = MAX(score_new, score_flush);
											}

											int score_old = 0;
											for (int np = 0; np < n_pocket_perms; np++)
												for (int nb = 0; nb < n_board_perms[k]; nb++)
													score_old = MAX(score_old, 
														(HR_old[HR_old[HR_old[board_paths[nb] + 
														c[5 + pocket_perms[np][0]]] + 
														c[5 + pocket_perms[np][1]]]]));

											n++;
											if (score_new != score_old)
											{
												std::cout << "\n" << "(" << c[0];
												for (int i = 1; i < 9; i++)
													std::cout << ", " << c[i];
												std::cout << "): old = " << score_old <<
													", new = " << score_new << "\n";
												goto fail;
											}
											else if ((n % 1000LL) == 0)
											{
												std::cout << "\r\t" << n << " / " << N[k] << 
													" combinations verified";
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		std::cout << "\r\t" << n << " / " << N[k] << " combinations verified";
	}

	std::cout << "\n\nAll combinations verified successfully.\n\n";
	std::cout << "\nGreat success.\n";
	free(HR_new); free(HR_old); HR_new = 0; HR_old = 0;
	return 0;

fail: 
	std::cout << "\nEpic fail.\n";
	free(HR_new); free(HR_old); HR_new = 0; HR_old = 0;
	return 1;
}

struct commas_locale : std::numpunct<char> 
{ 
	char do_thousands_sep() const { return ','; } 
	std::string do_grouping() const { return "\3"; }
};

int raygen9(const char *filename, const char *filename7, bool test=true)
{
	std::vector<int> hand_ranks;
	std::cout.imbue(std::locale(std::locale(), new commas_locale));
	generate_handranks(hand_ranks);
	smart_save(&hand_ranks[0], (int) hand_ranks.size(), filename);
	if (test && filename7)
		return test_all_handranks(filename, filename7);
	return 0;
}
