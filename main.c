/*
	Name: RM AND VM PROJECT
	Copyright: MIT License
	Author: Lukas Tyla & Kristupas Skabeikis
	Date: 30-03-14 16:20
	Description: Real and Virtual Machine project for Operating Systems course @ VU MIF
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define RM_MEMORY_ARRAY_LENGTH 300
#define MEMORY_ARRAY_WIDTH 4
#define FILE_FORMAT_MAX_LENGTH 100
#define OUTPUT_FILE_NAME "memory_status.txt"

int running = 0;
int asking = 1;
int i, j = 0, k = 0;

// array for supervisor memory
char rmSupervisorMemory[FILE_FORMAT_MAX_LENGTH][MEMORY_ARRAY_WIDTH];
// array for real machine operating memory
char rmMemory[RM_MEMORY_ARRAY_LENGTH][MEMORY_ARRAY_WIDTH];
// block array (block = 10 words = 10 memory blocks)
bool memoryBlocksUsed[RM_MEMORY_ARRAY_LENGTH/10] = {false};
// used to store the length of the program
int programLength = 0;
// determines the maximum amount of lines program can output
int maxOutputLines = 0;

// VM registries
char vm_R [4];
int  vm_IC = 0;
bool vm_C = false;

// RM registries
char rm_R [4];
int  rm_IC = 0;
int  PLR  [4];
int  CHST1, CHST2, CHST3;
bool rm_C = false;
char MODE;


// RM interrupts
int PI;
int SI;
int TI;
int IOI;

/**
 * Initializes memory array, supervisor array and R registries with 0 characters
 */
void initiliazeMemory()
{
	for (i = 0; i < RM_MEMORY_ARRAY_LENGTH; ++i)
	{
		for (j = 0; j < MEMORY_ARRAY_WIDTH; ++j)
		{
			rmMemory[i][j] = '0';
		}
	}
	for (i = 0; i < RM_MEMORY_ARRAY_LENGTH/2; ++i)
	{
		for (j = 0; j < MEMORY_ARRAY_WIDTH; ++j)
		{
			rmSupervisorMemory[i][j] = '0';
		}
	}
	for (i = 0; i < MEMORY_ARRAY_WIDTH; ++i)
	{
		rm_R[i] = '0';
		vm_R[i] = '0';
	}
}
/**
 * Loads the program from supervisor memory into the real memory using PLR 
 * to choose the right memory block
 * Loads only the needed blocks, program name, begin and end symbols are dropped 
 * return:
 *		int -1 - if the program fails to load program into memory
 *		int 0  - if the program loading completed without errors 
 */
int loadProgramIntoMemory()
{
	int lineCount = 0;
	int nextPLRBlockIndex = 0, programBeginingWritten = 0;
	int blockNumber = findRealBlockByPLR(nextPLRBlockIndex++);
	if (blockNumber == -1)
	{
		return -1;
	}

	for (i = 0; i < programLength; ++i)
	{
		// if $BEG block was detected
		if (rmSupervisorMemory[i][0] == '$' && rmSupervisorMemory[i][1] == 'B' && rmSupervisorMemory[i][2] == 'E' && rmSupervisorMemory[i][3] == 'G')
		{
			//printf("$BEG detected!\n");
		}
		// if $END block was detected
		else if(rmSupervisorMemory[i][0] == '$' && rmSupervisorMemory[i][1] == 'E' && rmSupervisorMemory[i][2] == 'N' && rmSupervisorMemory[i][3] == 'D')
		{
			return 0;
		}
		// if 00x0 line detected, stating the max output line count
		else if(isdigit(rmSupervisorMemory[i][0]) && isdigit(rmSupervisorMemory[i][1]) && isdigit(rmSupervisorMemory[i][2]) && isdigit(rmSupervisorMemory[i][3]) && rmSupervisorMemory[i][2] > '0' && programBeginingWritten == 0)
		{
			//printf("Set max output line count detected!\n");
			maxOutputLines = (rmSupervisorMemory[i][0] - '0') * 1000 + (rmSupervisorMemory[i][1] - '0') * 100 + (rmSupervisorMemory[i][2] - '0') * 10 + (rmSupervisorMemory[i][3] - '0');
		}
		// if all four members within array were NOT numbers - letters
		else if(!isdigit(rmSupervisorMemory[i][0]) && !isdigit(rmSupervisorMemory[i][1]) && !isdigit(rmSupervisorMemory[i][2]) && !isdigit(rmSupervisorMemory[i][3]) && programBeginingWritten == 0)
		{
			//printf("Program name part detected!\n");
		}
		// if $0x0 block detected, meaning - go to x block and put upcoming data there
		else if(rmSupervisorMemory[i][0] == '$' && rmSupervisorMemory[i][1] == '0' && isdigit(rmSupervisorMemory[i][2]) && rmSupervisorMemory[i][3] == '0' && rmSupervisorMemory[i][2] > 0)
		{
			//printf("$0x0 block detected\n");
			blockNumber = findRealBlockByPLR(rmSupervisorMemory[i][2] - '0');
			programBeginingWritten = 1; 
			lineCount = 0;
		}
		// otherwise - put to memory
		else
		{
		  	if (lineCount <= 9)
			{
				//printf("%d  %d", lineCount, blockNumber);
		   		for (j = 0; j < MEMORY_ARRAY_WIDTH; ++j)
		   		{
			    	rmMemory[lineCount + blockNumber * 10][j] = rmSupervisorMemory[i][j];
			    	//printf("%c", rmMemory[lineCount + blockNumber * 10][j]);
		   		}
		   		++lineCount;
		   		//printf("\n");
		   	}
	  		else
	  		{
	  			blockNumber = findRealBlockByPLR(nextPLRBlockIndex++);
	  			if (blockNumber == -1){
					return -1;
				}
	  			lineCount = 0;
	  		}
  		}
  	}
  	return -1;
}
/**
 * Return the real block for the provided virtual machine block index
 * variable:
 *		int PLRAddress - the index of the Virtual Machine memory block index
 * return:
 *		int -1         - if it was tried to find the real address of non existant memory block
 *		int PLRAddress - number of the real block adress
 */
int findRealBlockByPLR(int PLRAddress)
{
	if (PLRAddress > 9)
	{
		return -1;
	}
	else
	{
		char digit1 = rmMemory[(PLR[2] * 10 + PLR[3]) * 10 + PLRAddress][0];
		char digit2 = rmMemory[10 * (PLR[2] * 10 + PLR[3]) + PLRAddress][1];
		//printf("%c %c\n", digit1, digit2);
		return ( (digit1 - '0') * 10 + (digit2 - '0') );
	}
}
/**
 * Finds a random not used (not marked with - or marked as used) block
 * return:
 * 		int randomNumber - the number of the chosen free block
 *		int           -1 - if the function could not choose a random free block within 200 tries
 */
int getRandomFreeBlock()
{
	int chosing = 1;
	srand(time(NULL));
	int randomNumber, timesTried = 0;

	while (chosing != 0)
	{
		randomNumber = rand() % RM_MEMORY_ARRAY_LENGTH/10;
		if (memoryBlocksUsed[randomNumber] == true && rmMemory[randomNumber * 10][2] == '-')
		{
			randomNumber = rand() % RM_MEMORY_ARRAY_LENGTH/10;
			++timesTried;
		}
		else
		{
			chosing = 0;
		}
		//printf("%d  ", randomNumber);
		if (timesTried >= RM_MEMORY_ARRAY_LENGTH)
		{
			return -1;
		}
	}
	//printf("Tries: %d\n", timesTried);
	return randomNumber;
}
/**
 * output the RM memory contents to file
 * variable: 
 *		char* fileName - the name of the file to push the Real Machine memory contents
 * return:
 * 		int  0 - if the output process was succesful
 * 		int -1 - if there were problems outputing the memory contents
 */
int outputRealMachineMemoryToFile(char* fileName)
{
	FILE *outputFile = fopen(fileName, "w+");
	if (outputFile == NULL)
	{
		printf("Failed to create file \"%s\"\n", fileName);
		return 1;
	}
	int i, j;
	for (i = 0; i < RM_MEMORY_ARRAY_LENGTH; ++i)
	{
		fprintf(outputFile, "%03d: ",i);
		for (j = 0; j < MEMORY_ARRAY_WIDTH; ++j)
		{
			fprintf(outputFile, "%c", rmMemory[i][j]);
		}
		fprintf(outputFile, "\n");
	}
	fclose(outputFile);

	return 0;
}
/**
 * read the provided program file contents into supervisor memory
 * return:
 *		int  0 - if the reading was successful 
 *		and the program begins with "$BEG" and ends with "$END"
 *		int -1 - if there were problems reading the program file 
 *		or it does not begin with "$BEG" or end with "$END"
 */
int readProgramFile()
{
	FILE *programFile = NULL;
	char buffer;
	j = 0, k = 0;

	while (asking == 1)
	{
		char programFileName[20];
		printf("\nProvide the name of your program file to be run (upto 20 symbols): ");
		scanf("%s", programFileName);

		programFile = fopen(programFileName, "r+");

		if (programFile == NULL)
		{
			printf("There was an error opening program file \"%s\"!\n", programFileName);
		}
		else
		{
			printf("Program file \"%s\" opened successfully!\n", programFileName);
			asking = 0;
		}
	}	
	while (!feof(programFile))
	{
		for (i = 0; i <= MEMORY_ARRAY_WIDTH; ++i)
		{
			// get one symbol of code
			buffer = fgetc(programFile);
			// put that one symbol of code inside the array
			//if (buffer != '\n')
			rmSupervisorMemory[j][i] = buffer;
		}
		++j;
		// print the array contents char by char
		for (i = 0; i <= MEMORY_ARRAY_WIDTH; ++i)
		{
			printf("%c", rmSupervisorMemory[k][i]);
		}
		++k;
	}
	fclose(programFile);
	programLength = k;

	if (rmSupervisorMemory  [0][0] == '$' && 
		rmSupervisorMemory  [0][1] == 'B' &&
		rmSupervisorMemory  [0][2] == 'E' &&
		rmSupervisorMemory  [0][3] == 'G' &&
		rmSupervisorMemory[k-1][0] == '$' &&
		rmSupervisorMemory[k-1][1] == 'E' &&
		rmSupervisorMemory[k-1][2] == 'N' &&
		rmSupervisorMemory[k-1][3] == 'D'
		)
	{
		return 0;
	}
	else
	{
		return -1;
	}

}
/**
 * Displays the contents of the current registries state 
 */
void showRegistryStatus()
{
	printf("R: %c%c%c%c\n", vm_R[0], vm_R[1], vm_R[2], vm_R[3]);
	printf("C: %s\n", vm_C ? "true" : "false");
	printf("IC: %d\n", vm_IC);
	printf("PLR: %d%d%d%d\n", PLR[0], PLR[1], PLR[2], PLR[3]);
}
/**
 * Marks all words within the provided block with "-" and sets the block status tu used = true;
 */
void reserveBlock(int blockIndex) 
{
	int wordsFrom = blockIndex * 10;
	int wordsTo   = blockIndex * 10 + 10;
  
	for (i = wordsFrom; i < wordsTo; ++i) 
	{
		for (j = 0; j < MEMORY_ARRAY_WIDTH; ++j) 
		{
			rmMemory[i][j] = '-';
		}
	}
    memoryBlocksUsed[blockIndex] = true;
}
/**
 * Reserves space for paging table and sets the registry to point to that table
 * return:
 *		int blockNum - the number of the block reserved for paging table
 *		int -1 		 - if the paging table could not be initialized
 */
int initPLR()
{
    int blockNum = getRandomFreeBlock();
    if(blockNum == -1)
    {
        printf("FATAL ERROR: there is no space for paging table.");
        return  -1;
    }
    else
    {
    	reserveBlock(blockNum);
        PLR[0] = 0;
        PLR[1] = 0;
        PLR[2] = blockNum / 10;
        PLR[3] = blockNum % 10;
    }
    return blockNum;
}
/**
 * Calls the initPLR function to initialize Paging table and chooses 10 other memory blocks for program
 * contents and marks those random blocks in the paging table
 * return: 
 *		int -1 - if the PLRBlock could not be created
 */
int initializePageTable() 
{
    int PLRBlockIndex = initPLR();
    int m, x = 0;
    int intString[2];
    if (PLRBlockIndex != -1)
    {
	    for (m = 0; m < 10; ++m) 
	    {
	        int randomBlockIndex = getRandomFreeBlock();
	        //int isBlockEmpty = -1;

	        //isBlockEmpty = isBlockEmptyFunc(randomBlockIndex);
	    	reserveBlock(randomBlockIndex);
	    	intString[0] = randomBlockIndex / 10;
	    	//if (randomBlockIndex > 9) 
	    	intString[1] = randomBlockIndex % 10;
	    	char dig = '0' + intString[0];
	    	//printf("%d: %c ", randomBlockIndex, dig);
	    	rmMemory[PLRBlockIndex * 10 + m][0] = dig;
	    	dig = '0' + intString[1];
	    	//printf("%c\n", dig);
	    	rmMemory[PLRBlockIndex * 10 + m][1] = dig;

	    }
    }
    else
    {
    	return -1;
    }
}
/**
 * Returns the Real Address of a block given by x1 and x2
 */
int findRealAddress(int x1, int x2)
{
	char digit1 = rmMemory[(PLR[2] * 10 + PLR[3]) * 10 + x1][0];
	char digit2 = rmMemory[(PLR[2] * 10 + PLR[3]) * 10 + x1][1];
	int result = ( (digit1 - '0') * 100 + 10 * (digit2 - '0') + x2 );
	return result;
}
void commandPD(int x1)
{
	for (i = 0; i < 10; ++i)
	{
    	printf("rmMemory value: %c%c%c%c\n", rmMemory[findRealAddress(x1, i)][0], rmMemory[findRealAddress(x1, i)][1], 
                rmMemory[findRealAddress(x1, i)][2], rmMemory[findRealAddress(x1, i)][3]);
  	}
}

void commandAD(char x1, char x2)
{
	int suma = (vm_R[0] - '0') * 1000 +
             (vm_R[1] - '0') * 100 +
             (vm_R[2] - '0') * 10 +
             (vm_R[3] - '0') +
           	 (rmMemory[findRealAddress(x1 - '0', x2 - '0')][0] - '0') * 1000 +
             (rmMemory[findRealAddress(x1 - '0', x2 - '0')][1] - '0') * 100 +
             (rmMemory[findRealAddress(x1 - '0', x2 - '0')][2] - '0') * 10 +
             (rmMemory[findRealAddress(x1 - '0', x2 - '0')][3] - '0');
	  vm_R[3] = suma % 10 + '0';
	  vm_R[2] = suma % 100 / 10 + '0';
	  vm_R[1] = suma % 1000 / 100 + '0';
	  vm_R[0] = suma % 10000 / 1000 + '0';
}

void commandLR(char x1, char x2)
{
	for (i = 0; i < MEMORY_ARRAY_WIDTH; ++i)
	{
		vm_R[i] = rmMemory[findRealAddress(x1 - '0', x2 - '0')][i];
	}
}

void commandSR(char x1, char x2)
{
	for (i = 0; i < MEMORY_ARRAY_WIDTH; i++)
	{
		rmMemory[findRealAddress(x1 - '0', x2 - '0')][i] = vm_R[i];
	}
}

void commandCR(char x1, char x2)
{
	for (i = 0; i < MEMORY_ARRAY_WIDTH; ++i)
	{
		if(rmMemory[findRealAddress(x1 - '0', x2 - '0')][i] == vm_R[i])
			vm_C = true;
		else 
			vm_C = false;
	}
}

void commandBT(char x1, char x2)
{
	if (vm_C)
		vm_IC = (x1 - '0') * 10 + x2 - '0';
}

void commandGD(char x1)
{
	char buffer[40] = {};
	char input[10][4] = {};
	scanf("Įveskite iki 40 ženklų: %s", buffer);

	for (i = 0; i < 10; ++i)
	{
		for (j = 0; j < MEMORY_ARRAY_WIDTH; ++j)
		{
			rmMemory[findRealAddress(x1 - '0', i)][j] = buffer[i + (4 * i + j)];
		}
	}
}
void commandHALT()
{
	printf("HALT command executed. Virtual Machine work stopped!\n");
}
/**
 * SORT OF WORKING.
 */
void detectCommand()
{
	char command[4];

	while ( (command[0] != 'H' || command[1] != 'A' || command[2] != 'L' || command[3] != 'T') )
	{
		for (i = 0; i < MEMORY_ARRAY_WIDTH; ++i)
		{
			// send the line index
			command[i] = rmMemory[findRealAddress(vm_IC / 10, vm_IC % 10)][i];
		}
		if (command[0] == '-' && command[1] == '-')
		{
			vm_IC = vm_IC / 10 * 10;
			vm_IC += 10;
		}
		else
		{
			printf("Command: %c%c%c%c\n", command[0], command[1], command[2], command[3]);

			++vm_IC;

			switch(command[0])
			{
				case 'P':
					if (command[1] == 'D' && isdigit(command[2]) && isdigit(command[3]))
					{
						printf("PD command detected.\n");
						commandPD(command[2] - '0');
					}
					break;
				case 'A':
					if (command[1] == 'D' && isdigit(command[2]) && isdigit(command[3]))
					{
						printf("AD command detected.\n");
						commandAD(command[2], command[3]);
						showRegistryStatus();
					}
					break;
				case 'L':
					if (command[1] == 'R' && isdigit(command[2]) && isdigit(command[3]))
					{
						printf("LR command detected.\n");
						commandLR(command[2], command[3]);
						showRegistryStatus();
					}
					break;
				case 'S':
					if (command[1] == 'R' && isdigit(command[2]) && isdigit(command[3]))
					{
						printf("SR command detected.\n");
						commandSR(command[2], command[3]);
					}
					break;
				case 'C':
					if (command[1] == 'R' && isdigit(command[2]) && isdigit(command[3]))
					{
						printf("CR command detected.\n");
						commandCR(command[2], command[3]);
						showRegistryStatus();
					}
					break;
				case 'B':
					if (command[1] == 'T' && isdigit(command[2]) && isdigit(command[3]))
					{
						printf("BT command detected.\n");
						commandBT(command[2], command[3]);
						showRegistryStatus();
					}
					break;
				case 'G':
					if (command[1] == 'D' && isdigit(command[2]) && isdigit(command[3]))
					{
						printf("GD command detected.\n");
						commandGD(command[2]);
					}
					break;
				default:
					if (!isdigit(command[0]) && !isdigit(command[1]) && !isdigit(command[2]) && !isdigit(command[3]))
					{
						printf("Output string detected.\n");
					}
					break;
			}// end switch
		}// end else
	}// end while 
	printf("Program work done. HALT detected!\n");
}// end detectCommand()

/*------------------------------MAIN PROGRAM--------------------------------------*/
int main()
{	
	printf("/***************************************************************/\n");
	printf("* Welcome to Real Machine (RM) and Virtual Machine (VM) project!\n");
	printf("* This is a program which simulates VM work on top of RM.\n");
	printf("/***************************************************************/\n");

	printf("System Information:\n");

	initiliazeMemory();
	initializePageTable();

	if (readProgramFile() == 0)
	{
		printf("\nProgram file loading into supervisor memory was successful!\n");
	}
	else
	{
		printf("\nThere was a problem loading program into supervisor memory!\n");
		return -1;
	}
	if (loadProgramIntoMemory() == 0)
	{
		printf("\nProgram file loading into real memory was successful!\n");
	}
	else
	{
		printf("\nThere was a problem loading program into real memory!\n");
		return -1;
	}
	showRegistryStatus();
	detectCommand();

	if (outputRealMachineMemoryToFile(OUTPUT_FILE_NAME) == 0)
	{
		printf("Real machine memory was succesfully outputted to file!\n");
	}
	else
	{
		printf("There was a problem outputing the RM memory to file!\n");
		return -1;
	}

	//getchar();
	return 0;
}
