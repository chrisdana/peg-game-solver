################################################################################
# peg-game-solver.py
# Solves the peg game described in https://github.com/chrisdana/watch-peg-game
# with peg positions ordered from top to bottom, left to right (shown below).
# Game boards with 4, 5, or 6 rows are supported.
# 
#                 0
#              1     2
#           3     4      5
#        6     7      8     9
#     10    11    12    13    14
#  15    16    17    18    19    20
#
################################################################################

import networkx as nx
import sys

def triangular_number(n: int) -> int:
    """
    Computes the triangular number T(n).
    
    Parameters:
        n (int): The integer for which triangular number is to be calculated.
        
    Returns:
        int: Triangular number T(n).
    """
    return n * (n + 1) // 2

def generate_triangle_graph(n: int) -> nx.Graph:
    """
    Generates a graph representing T(n) objects arranged in an equilateral triangle.
    
    Parameters:
        n (int): The number of rows in the equilateral triangle.
        
    Returns:
        nx.Graph: Graph representing the equilateral triangle.
    """
    graph = nx.Graph()
    vertices = triangular_number(n)
    levels = []
    
    # Add vertices
    for i in range(vertices):
        graph.add_node(i)
    
    # Add edges
    for i in range(n):
        for j in range(i + 1):
            current_node = i * (i + 1) // 2 + j
            # Save the level this node is at
            graph.nodes[current_node]["level"] = i
            if i < n - 1:
                graph.add_edge(current_node, current_node + i + 1)  # Edge to bottom-left node
                graph.add_edge(current_node, current_node + i + 2)  # Edge to bottom-right node
            if j < i:
                graph.add_edge(current_node, current_node + 1)      # Edge to right node
    
    return graph


def get_valid_moves(graph: nx.Graph, bs: [bool]) -> [(int, int, int)]:
    """
    Returns an array of all valid moves of the current board state
    
    Parameters:
        graph (nx.Graph): Input graph.
        bs (list): The current board state as an array of True/False
        
    Returns:
        list: List containing all valid moves (src, peg_to_remove, dest)
    """
    moves = []
    for src in range(len(list(graph.nodes))):
        has_peg = bs[src]
        if has_peg:
            # A valid move is if a neighbor has a peg, but the next hole 
            # in the same direction is empty.
            for mid in list(graph.neighbors(src)):
                # Avoid cycles
                if mid == src:
                    continue
                
                # Neighbor must have a peg
                if not bs[mid]:
                    continue
                
                src_lvl = graph.nodes[src]["level"]
                mid_lvl = graph.nodes[mid]["level"]
                
                # There are two cases: Jumping between levels and jumping
                # horizontally in the same level
                if src_lvl != mid_lvl:
                    dest = (2 * mid) - src + 1
                else:
                    dest = (2 * mid) - src
                
                # Dest must be valid
                if dest not in graph.neighbors(mid):
                    continue
                
                # Dest cannot be a neighbor of src (avoids invalid (4, 2, 1) move)
                if dest in graph.neighbors(src):
                    continue
                
                # Ensure horizontal jumps are actually on the same level
                if src_lvl == mid_lvl and mid_lvl != graph.nodes[dest]["level"]:
                    continue
                
                # Dest must not have a peg
                if bs[dest]:
                    continue
                
                moves.append((src, mid, dest))
    return moves
            
def solve(graph: nx.Graph, bs: [bool]) -> bool:
    """
    Tries to solve a given triangle puzzle by depth-first search
    
    Parameters:
        graph (nx.Graph): Input graph.
        bs (list): The current board state (peg locations) as an array of True/False
        
    Returns:
        True if successful, False if not
        If successful the FINAL_MOVE_LIST will contain the solution
    """
    global FINAL_MOVE_LIST
    global N_MOVES
    global SEEN
    
    # If we have one peg left, we are done
    if bs.count(True) == 1:
        return True
    
    moves = get_valid_moves(graph, bs)
    
    # If there are no valid moves, return
    if len(moves) == 0:
        return False
    
    # Depth-first search 
    for m in moves:
        N_MOVES += 1
        new_bs = bs.copy()
        
        # Make the move indicated 
        new_bs[m[0]] = False
        new_bs[m[1]] = False
        new_bs[m[2]] = True
        
        # Encode board state in hashable form 
        bs_hash = ''.join(['1' if x else '0' for x in new_bs])
        
        # Skip if we have seen this board before
        if bs_hash in SEEN:
            continue
        
        SEEN.add(bs_hash)
        FINAL_MOVE_LIST.append(m)
        
        if solve(graph, new_bs) == True:
            return True
        else:
            FINAL_MOVE_LIST.pop()
                
    # Valid moves led to board states we have already seen
    return False

def print_bs(bs: [bool], n_rows: int) -> None:
    """
    Prints the board state

    Parameters:
        bs ([Bool]): List of boolean values representing peg locations
        n_rows (Int): Number of rows in the triangle
        
    Returns:
        None
    """
    next_row_idx = 0
    next_row_len = 1
    print("Board state (X - Hole, O - Peg):")
    for i in range(len(bs)):
        if i == next_row_idx:
            print("")
            print(" " * (n_rows - next_row_len), end="")
            next_row_idx += next_row_len
            next_row_len += 1
        print("%s " %("O" if bs[i] else "X"), end="")
    print("\n")
        
def usage() -> None:
    print("Error: Argument invalid.", file=sys.stderr)
    print("First argument must be number of rows in the triangle", file=sys.stderr)
    print("The number of rows must be in the range 4-6", file=sys.stderr)
    print("Example: python3 peg-game-solver.py 6", file=sys.stderr)

def main() -> int:
    global FINAL_MOVE_LIST
    global N_MOVES
    global SEEN

    FINAL_MOVE_LIST = []
    N_MOVES = 0
    SEEN = set()

    # Number of levels in the triangle (only works for N = [4,5,6])
    try:
        n_rows = int(sys.argv[1])
        if n_rows < 4 or n_rows > 6:
            raise
    except Exception:
        usage()
        sys.exit(1)
    
    triangle_graph = generate_triangle_graph(n_rows)
      
    # Initial board states can be reduced due to symmetry
    # We only need check (n_rows // 2) + 1 levels 
    # In each level we only need check (Xi // 2) + 1 nodes if Xi is the number of nodes in the ith level
    for i in range((n_rows // 2) + 1):
        for j in range(((i + 1) // 2) + 1):
            current_node = i * (i + 1) // 2 + j
            
            print("Trying inital board state with peg %d removed" % current_node)
            # Remove the initial peg and run the solver
            board_state = [True] * len(triangle_graph.nodes)
            board_state[current_node] = False
            print_bs(board_state, n_rows)
            
            if solve(triangle_graph, board_state):
                print("Solution:")
                for i, move in enumerate(FINAL_MOVE_LIST):
                    print("Move %d: %d --> %d" %((i + 1), move[0], move[2]))
                sys.exit(0)
            
            print("No solution found from this starting position.\n")

    print("Unable to solve puzzle. ")
    sys.exit(0)

if __name__ == '__main__':
    main()
