cmake_minimum_required(VERSION 3.15)

project(PegGameSolver 
    VERSION 1.0 
    DESCRIPTION "Peg game solver"
    LANGUAGES C
    )

add_executable(peg-game-solver 
    peg-game-solver.c
    )