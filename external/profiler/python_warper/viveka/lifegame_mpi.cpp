/************************************************************************************
 *  Name: Ke Fan                                                                    *
 *  BlazerId: kefan                                                                 *
 *  Course Section: CS 732                                                          *
 *  Homework #: 4 (MPI: Game of life)                                               *
 *  Date: Mar 30, 2020                                                              *
 *                                                                                  *
 *                                                                                  *
 *  To Compile: mpicc lifegame.c -o game_mpi (to print matrices add -DDEBUG_PRINT)  *
 *  To run: mpirun -n <number of processes> ./game_mpi <size> <max generations>       *
 *                                                                                  *
 ************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#include <mpi.h>

#include <iostream>
#include <string>

#include "logging_api.h"

int ntimestep;
int curTs;
int nprocs;
int rank;
std::string namespath; // call path of functions

void randomInitBoard(int *b, int N, int M);
void printBoard(int *b, int N, int M);
void copyGhostCell(int *b, int N, int M);
void printMyBoard(int *b, int N, int M);

// Main entry
int main(int argc, char **argv)
{
    // Check the number of arguments
    if (argc != 3) {
        printf("Usage: %s <N> <max generations>\n", argv[0]);
        exit(-1);
    }

    // MPI Initial
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS)
        printf("ERROR: MPI_Init error\n");
    if (MPI_Comm_size(MPI_COMM_WORLD, &nprocs) != MPI_SUCCESS)
        printf("ERROR: MPI_Comm_size error\n");
    if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS)
        printf("ERROR: MPI_Comm_rank error\n");


    for (int t = 0; t < 1; t++)
    {

    set_timestep(t, 1);
    set_rank(rank, nprocs);
    set_namespath("");

	Events e("main"); //"size:100"
	// e.set_self_pair("size:100"); //;memcost:200

    int *board, *counts, *offset, *my_board, *my_next;
    int N, maxGeneration, my_count, my_N;

    // Get arguments
    {
    	Events e("Pre");

		N = atoi(argv[1]);
		maxGeneration = atoi(argv[2]);

		{
			Events e("InitBoard", "COMP");
			// Mallocate memory for board and initialize
			board = (int*)malloc((N+2)*(N+2)*sizeof(int));
			randomInitBoard(board, N, N);
		}


		// Print initial board
		#ifdef DEBUG_PRINT
		if(rank == 0)
		{
			printf("Initial Board\n");
			printBoard(board, N, N);
		}
		#endif

		// Calculate the counts and offset of data per process
		{
			Events e("calCounts", "COMP");
			counts = (int*)malloc(sizeof(int)*nprocs);
			offset = (int*)malloc(sizeof(int)*nprocs);

			for (int i = 0; i < nprocs; i++)
				counts[i] = (N/nprocs + ((i<N%nprocs)?1:0))*(N+2);

			offset[0] = 0;
			for (int i = 1; i < nprocs; i++)
				offset[i] = offset[i-1] + counts[i-1];

			my_count = counts[rank];
			my_N = my_count/(N+2);

			// Mallocate memory for local current and next board per process
			my_board = (int*)malloc((my_count+2*(N+2))*sizeof(int));
			my_next = (int*)malloc((my_count+2*(N+2))*sizeof(int));
		}
    

		{
			Events e("scatter", "COMP");
			// Scatter data per process by row
			MPI_Scatterv(&board[N+2], counts, offset, MPI_INT, &my_board[N+2], my_count, MPI_INT, 0, MPI_COMM_WORLD);

			// Copy ghost cell
			copyGhostCell(my_board, my_N, N);
		}
    }

    // Set start time
    double starttime = MPI_Wtime();
    
    {
    	Events e("lifeChange");

		// Set prev and next process of a process
		int prev = rank - 1;
		int next = rank + 1;
		if (rank == 0)  prev = nprocs - 1;
		if (rank == (nprocs - 1))  next = 0;

		MPI_Status stats[2];

		// Iterations Begin
		for (int k = 0; k <= maxGeneration; k++)
		{
			int change_flag = 0;

			{
				Events e("exchange", "COMM", 2, k);

				// If the nprocs is larger than 1, send first row and last row to prev and next process
				if(nprocs != 1)
				{
					// Sync each process
					MPI_Barrier(MPI_COMM_WORLD);

					// Blocking point-to-point communication
					MPI_Sendrecv(&my_board[N+2], N+2, MPI_INT, prev, 0, &my_board[my_count+(N+2)], N+2, MPI_INT, next, 0, MPI_COMM_WORLD, &stats[0]);
					MPI_Sendrecv(&my_board[my_count], N+2, MPI_INT, next, 1, my_board, N+2, MPI_INT, prev, 1, MPI_COMM_WORLD, &stats[1]);
				}
				else
				{
					// if there is only one process, just copy the ghost cell
					for (int i = 0; i < N+2; i++)
					{
						my_board[i] = my_board[N*(N+2)+i];
						my_board[(N+1)*(N+2)+i] = my_board[N+2+i];
					}
				}

				// Print current board
				#ifdef DEBUG_PRINT
				for(int r = 0; r < nprocs; r++)
				{
					if(rank == r)
					{
						printf("\nRank: %d, Generation: %d\n", rank, k);
						printMyBoard(my_board, my_N, N);
					}
				}
				#endif

			}


			{
				Events e("BoardChange", "COMP", 2, k);

				for (int i = 1; i < my_N+1; i++)
				{
					for (int j = 1; j < N+1; j++)
					{
						// Calculate number of alive neighbors
						int alives = my_board[(i-1)*(N+2)+(j-1)] + my_board[(i-1)*(N+2)+(j)]
						+ my_board[(i-1)*(N+2)+(j+1)] + my_board[i*(N+2)+(j-1)]
						+ my_board[i*(N+2)+(j+1)] + my_board[(i+1)*(N+2)+(j-1)]
						+ my_board[(i+1)*(N+2)+(j)] + my_board[(i+1)*(N+2)+(j+1)];

						// Print alives per process
						#ifdef DEBUG_PRINT
						for(int r = 0; r < nprocs; r++)
						{
							if(rank == r)
							{
								printf("r:%d,n:%d ", rank, alives);
							}
						}
						#endif

						// Change status of cell based on the current status and the number of alives
						if (my_board[i*(N+2)+j] == 1 && (alives < 2 || alives > 3))
						{
							my_next[i*(N+2)+j] = 0;
							change_flag = 1;
						}
						else if (my_board[i*(N+2)+j] == 0 && alives == 3)
						{
							my_next[i*(N+2)+j] = 1;
							change_flag = 1;

						}
						else
						{
							my_next[i*(N+2)+j] = my_board[i*(N+2)+j];
						}
					}
					#ifdef DEBUG_PRINT
					printf("\n");
					#endif
				}
			}

			{
				Events e("statusCheck", "COMP", 2, k);

				// Copy ghost cell
				copyGhostCell(my_next, my_N, N);

				// Sync each process and rank 0 gets the max change value
				int change_result;
				MPI_Barrier(MPI_COMM_WORLD);
				MPI_Reduce(&change_flag, &change_result, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

				// Print change result
				#ifdef DEBUG_PRINT
				if(rank == 0)
					printf("rank: %d, generation:%d, change_flag: %d\n", rank, k, change_result);
				#endif

				// If change result is 0, all the processes exit and print the taken time.
				if(rank == 0)
				{
					if (change_result == 0)
					{
						double end = MPI_Wtime() - starttime;
						printf("There is no any change between two generations!\n");
						printf("Time Taken: %f\n", end);
						MPI_Abort(MPI_COMM_WORLD, -1);
					}
				}

				// Change pointers per process
				int *tmp = my_board;
				my_board = my_next;
				my_next = tmp;
			}
		}
    }
    

    {
    	if (rank == 1)
    		Events e("gather", "COMP");
		// Sync each process, gather the final board, and calculate the taken time
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Gatherv(my_board, my_count, MPI_INT, board, counts, offset, MPI_INT, 0, MPI_COMM_WORLD);
    }


//    double endtime = MPI_Wtime() - starttime;
//    MPI_Reduce(&endtime, &starttime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    
    // Print the taken time
//    if(rank == 0)
//    {
//        printf("Max genration has been reached!\n");
////        printf("Time Taken: %f\n", starttime);
//    }
    
//    e.printMessage();

    /* Quit */
    free(my_board);
    free(my_next);

    }

    write_output("lifeGame_loop100");

    MPI_Finalize();
    return 0;
}


void randomInitBoard(int *b, int N, int M)
{
    // Generate random start point
    srand((unsigned int)time(NULL));
    // Initial N*N board randomly (except ghost cells)
    for (int i = 1; i < N+1; i++)
    {
        for (int j = 1; j < M+1; j++)
        {
            // Generate random number between (0,10)
            int cell = rand()%10;
            // 1 means alive cell, 0 means dead
            if (cell < 3)
                b[i*(M+2) + j] = 1;
            else
                b[i*(M+2) + j] = 0;
        }
    }
}


// Print board
void printBoard(int *b, int N, int M)
{
    for (int i = 1; i < N+1; i++)
    {
        for(int j = 1; j < M+1; j++)
        {
            printf("%d ", b[i*(M+2) + j]);
        }
        printf("\n");
    }
}

// Copy ghost cell
void copyGhostCell(int *b, int N, int M)
{
    for(int i = 1; i < N+1; i++)
    {
        b[i*(M+2)] = b[i*(M+2)+M];
        b[i*(M+2)+(M+1)] = b[i*(M+2)+1];
    }
}

// Print the whole board which includes ghost cell
void printMyBoard(int *b, int N, int M)
{
    for (int i = 0; i < N+2; i++)
    {
        for(int j = 0; j < M+2; j++)
        {
            printf("%d ", b[i*(M+2) + j]);
        }
        printf("\n");
    }
}

