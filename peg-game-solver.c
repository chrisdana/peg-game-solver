/******************************************************************************
* peg-game-solver
* Solves the peg game described in https://github.com/chrisdana/watch-peg-game
* with peg positions ordered from top to bottom, left to right (shown below).
* Game boards with 4, 5, or 6 rows are supported.
* 
*                 0
*              1     2
*           3     4      5
*        6     7      8     9
*     10    11    12    13    14
*  15    16    17    18    19    20
*/

#include <stdio.h>
#include <stdlib.h>
# include <math.h>

#define MAX_NEIGHBORS 6
#define EMPTY -1


static int triangular_number(int n) {
    return (n * (n + 1) / 2);
}

static void add_directional_edge(int **graph, int n1, int n2) {
    int i;
    for (i = 0; i < MAX_NEIGHBORS; i++) {
        if (graph[n1][i] == EMPTY) {
            graph[n1][i] = n2;
            break;
        }
    }    
}

static void add_edge(int **graph, int n1, int n2) {
    add_directional_edge(graph, n1, n2);
    add_directional_edge(graph, n2, n1);
}

static int** gen_triangle_graph(int n_rows) {
    int n = triangular_number(n_rows);
    int i, j;
    int current_node;

    /* Allocate memory and initialize */
    int **g = malloc(n * sizeof(int*));
    for (i = 0; i < n; i++) {
        g[i] = malloc(MAX_NEIGHBORS * sizeof(int));
        for (j = 0; j < MAX_NEIGHBORS; j++) {
            g[i][j] = EMPTY;
        }
    }
    
    /* Fill in neighbors */
    for (i = 0; i < n_rows; i++) {
        for (j = 0; j < (i + 1); j++) {
            /* The first node in each row is the triangular number for that row */
            current_node = triangular_number(i) + j;
            if (i < (n_rows - 1)) {
                add_edge(g, current_node, current_node + i + 1); /* Edge ot lower-left */
                add_edge(g, current_node, current_node + i + 2); /* Edge to lower-right */
            }
            if (j < i) {
                add_edge(g, current_node, current_node + 1); /* Edge to right */
            }
        }
    }

    return g;
}

static int row_from_node(int n) {
    /* The ith row is the positive, floored solution to the equation i^2 + i - 2n = 0 */
    return (int)((-1 + sqrtf(1 + (4 * (2 * n)))) / 2);
}

static int has_peg(int n, uint32_t state) {
    return ((state >> n) & 0x01);
}

static void set_peg(int n, uint32_t *state) {
    *state = *state | (0x01 << n);
}

static void rem_peg(int n, uint32_t *state) {
    *state = *state & ~(0x01 << n);
}

static int enc_move(int src, int mid, int dest) {
    return (10000 * src) + (100 * mid) + dest;
}

static void dec_move(int move, int *src, int *mid, int *dest) {
    *src = move / 10000;
    *mid = (move / 100) % 100;
    *dest = move % 100;
}

static int count_pegs(uint32_t i) {
    int count = 0;

    while (i) {
        count += i & 1;
        i >>= 1;
    }
    
    return count;
}

static void print_bs(uint32_t bs, int n_nodes, int n_rows) {
    int next_row_idx = 0;
    int next_row_len = 1;
    printf("Board state (0 - Hole, 1 - Peg):");
    for (int i = 0; i < n_nodes; i++) {
        if (i == next_row_idx) {
            printf("\n");
            printf("%.*s", (n_rows - next_row_len), "          ");
            next_row_idx += next_row_len;
            next_row_len += 1;
        }
        printf("%d ", has_peg(i, bs));
    }
    printf("\n");
}

static int is_neighbor(int n, int k, int **graph) {
    for (int i = 0; i < MAX_NEIGHBORS; i++) {
        if (graph[n][i] == k) {
            return 1;
        }
        else if (graph[n][i] == EMPTY) {
            break;
        }
    }
    return 0;
}

/* Returns all valid moves of a given board state in *moves (if not NULL) */
static int get_valid_moves(int **graph, uint32_t state, int n_nodes, int moves[]) {
    int src, mid, dest;
    int i;
    int src_row, mid_row, dest_row;
    int n_moves = 0;
    int i_valid_moves = 0;

    for (src = 0; src < n_nodes; src++) {
        if (!(has_peg(src, state))) {
            continue;
        }

        for (i = 0; i < MAX_NEIGHBORS; i++) {
            if (graph[src][i] == EMPTY) {
                break;
            }

            mid = graph[src][i];

            if (mid == src) {
                continue;
            }

            if (!(has_peg(mid, state))) {
                continue;
            }

            src_row = row_from_node(src);
            mid_row = row_from_node(mid);

            // There are two cases: Jumping between levels and jumping
            // horizontally in the same level
            if (src_row != mid_row) {
                dest = (2 * mid) - src + 1;
            }
            else {
                dest = (2 * mid) - src;
            }

            // Dest must be positive
            if (dest < 0) {
                continue;
            }

            // Dest must be a neighbor of mid
            if (!(is_neighbor(mid, dest, graph))) {
                continue;
            }

            // Dest cannot be a neighbor of src (avoids invalid 4, 2, 1 move)
            if (is_neighbor(src, dest, graph)) {
                continue;
            }

            // Ensure horizontal jumps are on the same level
            dest_row = row_from_node(dest);
            if ((src_row == mid_row) && (mid_row != dest_row)) {
                continue;
            }

            // Dest must not have a peg
            if (has_peg(dest, state)) {
                continue;
            }

            n_moves++;
            if (moves != NULL) {
                int encoded = enc_move(src, mid, dest);
                //printf("Adding move: %d %d %d  (%d)\n", src, mid, dest, encoded);
                moves[i_valid_moves] = encoded;
                i_valid_moves++;
            }
        }
    }
    return n_moves;
}

static int n_valid_moves(int **graph, uint32_t state, int n_nodes) {
    return get_valid_moves(graph, state, n_nodes, NULL);
}

static int solve(int **graph, uint32_t curr_bs, int n_nodes, int *final_moves, int *idx_final_moves) {
    int n_moves = 0;
    int move;
    uint32_t bs = curr_bs;
    int src, mid, dest;

    /* If we have one peg left, we are done */
    if (count_pegs(bs) == 1) {
        return 1;
    }

    /* If there are valid moves, return */
    n_moves = n_valid_moves(graph, bs, n_nodes);
    if (n_moves == 0) {
        return 0;
    }

    /* Get all valid moves */
    int *moves = malloc(n_moves * sizeof(int));
    get_valid_moves(graph, bs, n_nodes, moves);

    /* Depth-first search */
    for (int i = 0; i < n_moves; i++) {
        // Reset the board state to initial function call
        bs = curr_bs;

        // Make the move
        move = moves[i];
        dec_move(move, &src, &mid, &dest);
        rem_peg(src, &bs);
        rem_peg(mid, &bs);
        set_peg(dest, &bs);

        // Add the move to the final move list
        final_moves[*idx_final_moves] = move;
        (*idx_final_moves)++;

        if (solve(graph, bs, n_nodes, final_moves, idx_final_moves) == 1) {
            return 1;
        } else {
            // Remove the last move
            (*idx_final_moves)--;
            final_moves[*idx_final_moves] = 0;
        }
    }

    // No solutions down this branch
    free(moves);
    return 0;
}

int main(int argc, char **argv) {
    int n_rows = 0;
    if (argc > 1) {
        n_rows = atoi(argv[1]);
    }

    if (n_rows < 4 || n_rows > 6) {
        fprintf(stderr, "Error: Argument invalid.\n");
        fprintf(stderr, "First argument must be number of rows in the triangle\n");
        fprintf(stderr, "The number of rows must be in the range 4-6\n");
        fprintf(stderr, "Example: ./peg-game-solver 6\n");
        exit(1);
    }

    int n_nodes = triangular_number(n_rows);
    int **graph = gen_triangle_graph(n_rows);
    int *final_moves = malloc(n_nodes * sizeof(int));
    int idx_final_moves = 0;
    int src, mid, dest;
    int curr_node;
    uint32_t init_bs;
    int ret = 0;

    // Set the initial board state
    for (int i = 0; i < (n_rows / 2) + 1; i++) {
        for (int j = 0; j < (((i + 1) / 2) + 1); j++) {
            curr_node = triangular_number(i) + j;

            printf("Trying initial state with peg %d removed\n", curr_node);
            init_bs = 0;
            for (int k = 0; k < n_nodes; k++) {
                set_peg(k, &init_bs);
            }
            rem_peg(curr_node, &init_bs);
            print_bs(init_bs, n_nodes, n_rows);

            ret = solve(graph, init_bs, n_nodes, final_moves, &idx_final_moves);
            if (ret == 1) {
                printf("Solution:\n");
                for (int i = 0; i < (n_nodes - 2); i++) {
                    dec_move(final_moves[i], &src, &mid, &dest);
                    printf("Move %d:  %d --> %d\n", (i + 1), src, dest);
                }
                free(final_moves); 
                exit(0);
            }
            printf("No solution found from this starting position.\n\n");
        }
    }

    printf("Unable to solve puzzle.\n");
    free(final_moves);
    exit(0);
}