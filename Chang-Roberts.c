#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <mpi.h>

/* 
 * Mainly used to prevent printing the same error msgs
 * multiple times by different processes and to allocate
 * UniqueID(UID) to processes only once.
 */
#define ZERO_RANK_PROCESS (0)

/* Define process state in the election */
#define PASSIVE_STATE (0)
#define ACTIVE_STATE (1)

/* Message packet size */
#define MSG_PKT_SZ (3)

/* Msg definitions */
#define ELECTION_MSG (0)
#define ELECTED_MSG (1)

/* Verbose printing */
#define PRINT_VERBOSE (0)

void Chang_Robert_Algorithm(int rank, int size, int initProc)
{
	uint32_t UID, sendToProcRank, leaderUID;
	uint32_t procState = PASSIVE_STATE;
	bool electionInProgress = true, leaderDetermined = false, electedMsgPassing = false;
	int instanceNumber = 0;
	MPI_Status status;
	/*
	 * msgPkt[0] : 0 -> election msg, 1 -> elected msg
	 * msgPkt[1] : UID
	 * msgPkt[2] : leaderUID to be sent in elected msg after election is done and leader is elected	
	 */
	int msgPkt[MSG_PKT_SZ];

	/* Intialize random number generator */
	srand(time(NULL) + rank);

	if(rank == ZERO_RANK_PROCESS) {
		printf("\n******\tChang and Roberts Election Algorithm Implementation\t******\n");
		usleep(2500);
	}

	/* Synchronize the ID allocation among all the processes */
	MPI_Barrier(MPI_COMM_WORLD);
	UID = rand() % 1000000;
	printf("Process Rank:  %d  <--->  Unique-Identifier (UID): %d\n", rank, UID);
	usleep(2500);

	/* 
	 * Synchronize and wait until the process rank is equal to the
	 * election initializer process rank
	 */
	MPI_Barrier(MPI_COMM_WORLD);

	if (rank == initProc) {
		sendToProcRank = (rank + 1) % size;
		printf("\nProcessor %d is initiating the election and sending the UID #%d to processor #%d\n\n", rank, UID, sendToProcRank);
		usleep(2500);

		/* Build message packet to be sent */
		msgPkt[0] = ELECTION_MSG;
		msgPkt[1] = UID; // UID of the process sending the MSG

		procState = ACTIVE_STATE;
		
		/* Part1 of the algorithm: Initial process starting the election process */
		MPI_Send((const void *)msgPkt, MSG_PKT_SZ, MPI_INT, sendToProcRank, 0, MPI_COMM_WORLD);
	}

	while (electionInProgress) {
		MPI_Recv((void *)msgPkt, MSG_PKT_SZ, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
		
		// Election finished! Now send out an elected message 
		if(msgPkt[0] == ELECTED_MSG) {
			procState = PASSIVE_STATE;
			leaderUID = msgPkt[2];
			electionInProgress = false;
			electedMsgPassing = true;
		} else { // Election in progress 
			if (msgPkt[1] == UID) {
				leaderDetermined = true;
				msgPkt[2] = UID;
				msgPkt[0] = ELECTED_MSG;
			} else if (msgPkt[1] > UID) {
				procState = PASSIVE_STATE;
			} else if (msgPkt[1] < UID) {
				procState = ACTIVE_STATE;
				msgPkt[1] = UID;
			}

			if (PRINT_VERBOSE) {
				printf("\nInstance number: %d\nProcess:\n\tRank: %d\n\tUID: %d\n\tState: %s\n\n",
						++instanceNumber, rank, UID, (procState == ACTIVE_STATE) ? "ACTIVE" : "PASSIVE");
				usleep(2500);
			}
		}

		sendToProcRank = (rank + 1) % size;
		// Send message to the next neighbour process 
		MPI_Send((const void *)msgPkt, MSG_PKT_SZ, MPI_INT, sendToProcRank, 0, MPI_COMM_WORLD); 
	}

	/* Part2 of the algorithm: Pass elected msg to all remaining nodes and put them to passive state */
	while (electedMsgPassing) {
		MPI_Recv((void *)msgPkt, MSG_PKT_SZ, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

		if (msgPkt[0] == ELECTED_MSG && UID != leaderUID) {
			sendToProcRank = (rank + 1) % size;
			procState = PASSIVE_STATE;
			MPI_Send((const void *)msgPkt, MSG_PKT_SZ, MPI_INT, sendToProcRank, 0, MPI_COMM_WORLD);
		} else if (msgPkt[0] == ELECTED_MSG && UID == leaderUID) {
			electedMsgPassing = false;
		}
	}

	if (leaderDetermined) {
		printf("\n******\tLeader!!! Rank: %d UID: %d\t******\n", rank, UID);
		usleep(2500);
	}
}

/* 
 * A process's rank is always between 0 and size-1.
 * This function validates that the process rank supplied is within acceptable bounds.
 */
bool InitElectProcessIsValid(const char *rank, int size)
{
	uint32_t processRank = strtoul(rank, NULL, 10);

	if (processRank >= size) {
		printf("Error: Rank of the process (#%d) provided is OOB..! must be <0 to size-1>\n", processRank);
		return false;
	}

	return true;
}

int main(int argc, char **argv)
{
	/* process rank and communicator size */
	int procRank, commSize;
	
	/* Initialize MPI */
	MPI_Init(&argc, &argv);

	/* Get the Rank of the process in the communicator*/
	MPI_Comm_rank(MPI_COMM_WORLD, &procRank);

	/* Get the size of the group associated with the communicator */
	MPI_Comm_size(MPI_COMM_WORLD, &commSize);

	/*
	 * To simulate the C&R election algorithm, three processes are a minimum. 
	 * Before performing the algorithm, this check ensures that the communicator's size is larger than 2.
	 */
	if(commSize < 3) { 
		if (procRank == ZERO_RANK_PROCESS)
			printf("Error: At least three processes are required to run this algorithm.\n");
		MPI_Finalize();
		return -EINVAL;
	}
	
	if (!InitElectProcessIsValid(argv[1], commSize)) {
		if (procRank == ZERO_RANK_PROCESS)
			printf("Error: Invalid process rank!\n");
		MPI_Finalize();
		return -EINVAL;
	}

	/* 
	 * User input: Primary process # that initiates the election
	 */
	uint32_t firstProc = strtoul(argv[1], NULL, 10);
	if (procRank == ZERO_RANK_PROCESS)
		printf("\nThe election to be initiated by the process rank #%d ...\n", firstProc);
	
	Chang_Robert_Algorithm(procRank, commSize, firstProc);

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	
	return 0;
}
