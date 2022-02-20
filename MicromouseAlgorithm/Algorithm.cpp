#include <iostream>
#include <cstdint>
#include <stack>

#include "Maze.h"
#include "PathFinder.h"
#include "MazeDefinitions.h"

using namespace std;

/* Constants */
#define MAZE_X 16
#define MAZE_Y 16

/* Clear Screen Prototype */
void clearScreen();

/* Coordinate Struct */
struct Coordinate
{
	Coordinate(int xInput, int yInput)
	{
		x = xInput; y = yInput;
	}
	unsigned x, y;
};

class Algorithm : public PathFinder
{
public:
	Algorithm();
	virtual MouseMovement nextMovement(unsigned x, unsigned y, const Maze &maze);
	inline void updateMaze(unsigned x, unsigned y, const Maze &maze);
	inline bool checkWall(unsigned x, unsigned y, int wallDir);
	inline void analyzePath(unsigned x, unsigned y, int dirLogic[]);
	inline MouseMovement mouseMvt(int dirLogic[]);
	inline void stackFloodFill(unsigned x, unsigned y);
	inline Dir adjustDirection(int relativeDir);
	inline Dir adjustTarget(int relativeDir);
	inline bool checkCenter(unsigned x, unsigned y);
	inline bool checkStart(unsigned x, unsigned y);

protected:
	uint16_t m_WallWE[MAZE_X];						// x-Axis = [x]
	uint16_t m_WallSN[MAZE_Y];						// Span(y-Axis) = Vector-Bit Data [uint16_t]
	unsigned m_dist[MAZE_X][MAZE_Y];				// FloodFill Distance Data
	stack<Coordinate> c_Stack;						// FloodFill Coordinate Stack
	bool c_Process[MAZE_X][MAZE_Y];					// Stack Process-Cell Check
	Dir heading;									// Mouse Direction
	bool mazeTarget;								// Mouse mazeTarget: True/False ~ Center/Start
	int runCount;									// Mouse Cycle Counter
	bool mazeSolved;								// Maze Solved (?) Boolean
	bool visited[MAZE_X][MAZE_Y];					// Visited Cells - Use To Be Determined
};

int main()
{
	// Construct Simulation Objects
	Algorithm MicroMouse;
	Maze maze(MazeDefinitions::MAZE_APEC_2013, &MicroMouse);

	// Simulation
	maze.start();

	return 0;
}

// Algorithm Maze Constructor
Algorithm::Algorithm()
{
	heading = NORTH;								// Initial Heading
	mazeSolved = false;								// Maze Solved?
	runCount = 0;									// Mouse Cycle Counter
	mazeTarget = true;								// Mouse mazeTarget: True/False ~ Center/Start

	// Initialize Empty Wall Field and Boundaries of Square Maze
	memset(m_WallWE, 0, sizeof(m_WallWE));
	memset(m_WallSN, 0, sizeof(m_WallSN));
	for (int x = 0; x < MAZE_X; x++)
		m_WallSN[x] |= 1;
	for (int y = 0; y < MAZE_Y; y++)
		m_WallWE[0] |= 1 << y;

	// Initialize FloodFill & bool c_Process/Visited for Square Maze
	for (int x = 0; x < MAZE_X / 2; x++)
	{
		// Bottom-Left
		for (int y = 0; y < MAZE_Y / 2; y++)
		{
			m_dist[x][y] = MAZE_X - 2 - x - y;
			c_Process[x][y] = false;
			visited[x][y] = false;
		}
		// Top-Left
		for (int y = MAZE_Y / 2; y < MAZE_Y; y++)
		{
			m_dist[x][y] = y - x - 1;
			c_Process[x][y] = false;
			visited[x][y] = false;
		}
	}
	for (int x = MAZE_X / 2; x < MAZE_X; x++)
	{
		// Bottom-Right
		for (int y = 0; y < MAZE_Y / 2; y++)
		{
			m_dist[x][y] = x - y - 1;
			c_Process[x][y] = false; 
			visited[x][y] = false;
		}
		// Top-Right
		for (int y = MAZE_Y / 2; y < MAZE_Y; y++)
		{
			m_dist[x][y] = x + y - MAZE_Y;
			c_Process[x][y] = false; 
			visited[x][y] = false;
		}
	}
}

// Mouse Algorithm Loop
MouseMovement Algorithm::nextMovement(unsigned x, unsigned y, const Maze &maze)
{
	// Simulation Visual & Pause
	clearScreen();
	cout << maze.draw(2) << endl;
	cin.ignore(10000, '\n');

	/* Micromouse Algorithm */
	// Macroscopic Mouse/Maze Logic
	if (runCount >= 3 && checkStart(x, y) && mazeSolved)
		return Finish;
	if (checkStart(x, y) && !mazeTarget)
		mazeTarget = true;
	if (checkCenter(x, y) && mazeTarget)
	{
		runCount++;
		mazeTarget = false;
		mazeSolved = true;
	}

	/* Mouse Artificial Intelligence */
	int dirLogic[4] = { INVALID, INVALID, INVALID, INVALID };
	if (mazeSolved)
	{
		updateMaze(x, y, maze);
		// Macroscopic Logic Update: False/True ~ mazeTarget Start/Center Switch
		static bool resetSwitch = false;
		if (mazeTarget)		 // mazeTarget -> Center													
		{
			// Reverse FloodFill Target & Move to Maze Center
			if (resetSwitch)
			{
				m_dist[MAZE_X / 2][MAZE_Y / 2] = (m_dist[MAZE_X / 2 - 1][MAZE_Y / 2] = (m_dist[MAZE_X / 2][MAZE_Y / 2 - 1] = (m_dist[MAZE_X / 2 - 1][MAZE_Y / 2 - 1] = 0)));
				stackFloodFill(x, y);
				resetSwitch = false;				// Flip Switch, No More Updates
			}
		}
		else	// mazeTarget -> Start
		{
			// Reverse FloodFill Target & Move Back to Starting Cell
			if (!resetSwitch)
			{
				m_dist[0][0] = 0;
				stackFloodFill(x, y);
				resetSwitch = true;			// Flip Switch, No More Updates
			}
		}
	}
	// Update Maze Walls and Push Coordinate to Stack
	updateMaze(x, y, maze);
	// Analyze Neighboring Cells for Movement Options		
	analyzePath(x, y, dirLogic);					
	// Mouse Stuck, Run FloodFill & Re-Analyze
	if (dirLogic[0] == INVALID)		
	{
		stackFloodFill(x, y);
		analyzePath(x, y, dirLogic);
	}
	// Execute Dynamic Movement
	return mouseMvt(dirLogic);																
}

// Updates NORTH/WEST/EAST Walls and Pushes Maze Unit on Stack(s)
inline void Algorithm::updateMaze(unsigned x, unsigned y, const Maze &maze)
{
	// Update Walls
	if (maze.wallInFront())
	{
		switch (adjustDirection(NORTH))
		{
		case NORTH:
			if (y == MAZE_Y - 1)
				break;
			m_WallSN[x] |= 1 << (y + 1);
			break;
		case SOUTH:
			m_WallSN[x] |= 1 << y;
			break;
		case EAST:
			if (x == MAZE_X - 1)
				break;
			m_WallWE[x + 1] |= 1 << y;
			break;
		case WEST:
			m_WallWE[x] |= 1 << y;
			break;
		default:
			break;
		}
	}
	if (maze.wallOnLeft())
	{
		switch (adjustDirection(WEST))
		{
		case NORTH:
			if (y == MAZE_Y - 1)
				break;
			m_WallSN[x] |= 1 << (y + 1);
			break;
		case SOUTH:
			m_WallSN[x] |= 1 << y;
			break;
		case EAST:
			if (x == MAZE_X - 1)
				break;
			m_WallWE[x + 1] |= 1 << y;
			break;
		case WEST:
			m_WallWE[x] |= 1 << y;
			break;
		default:
			break;
		}
	}
	if (maze.wallOnRight())
	{
		switch (adjustDirection(EAST))
		{
		case NORTH:
			if (y == MAZE_Y - 1)
				break;
			m_WallSN[x] |= 1 << (y + 1);
			break;
		case SOUTH:
			m_WallSN[x] |= 1 << y;
			break;
		case EAST:
			if (x == MAZE_X - 1)
				break;
			m_WallWE[x + 1] |= 1 << y;
			break;
		case WEST:
			m_WallWE[x] |= 1 << y;
			break;
		default:
			break;
		}
	}

	// Update STACK and Visited Mark
	if (!c_Process[x][y])
	{
		Coordinate data(x, y);
		c_Stack.push(data);
		c_Process[x][y] = true;
	}
	visited[x][y] = true;
}

// Check Wall in Direction wallDir at Cell Position (NSEW)
inline bool Algorithm::checkWall(unsigned x, unsigned y, int wallDir)
{
	switch (wallDir)
	{
	case NORTH:
		if (y == MAZE_Y - 1)
			return true;
		if ((m_WallSN[x] & 1 << (y + 1)) > 0)
			return true;
		return false;
	case SOUTH:
		if ((m_WallSN[x] & 1 << y) > 0)
			return true;
		return false;
	case EAST:
		if (x == MAZE_X - 1)
			return true;
		if ((m_WallWE[x + 1] & 1 << y) > 0)
			return true;
		return false;
	case WEST:
		if ((m_WallWE[x] & 1 << y) > 0)
			return true;
		return false;
	default:
		return true;
	}
}

// Analyzes Mouse Movement Options @ (x, y) -> Set Dir (Movement Direction) dirLogic[] Choice Priority
inline void Algorithm::analyzePath(unsigned x, unsigned y, int dirLogic[])
{
	/* FloodFill Movement Logic */
	// Manhattan Distance Memory
	unsigned distArray[4] = { INT_MAX, INT_MAX, INT_MAX, INT_MAX };		
	// Setup Mvt Options and Initialize FloodFill distArray to 4 Respective Directions
	dirLogic[0] = adjustTarget(NORTH);									
	dirLogic[1] = adjustTarget(SOUTH);
	dirLogic[2] = adjustTarget(EAST);
	dirLogic[3] = adjustTarget(WEST);
	if (y < MAZE_Y - 1)											
		distArray[0] = m_dist[x][y + 1];
	if (y > 0)
		distArray[1] = m_dist[x][y - 1];
	if (x < MAZE_X - 1)
		distArray[2] = m_dist[x + 1][y];
	if (x > 0)
		distArray[3] = m_dist[x - 1][y];
	// If FloodFill > Current Cell Floodfill, INVALID Mvt Option
	for (int i = 0; i < 4; i++)											
		if (distArray[i] >= m_dist[x][y])
			distArray[i] = INT_MAX;
	// Prefer Movement in Direction of Heading...Swap 1st Mvt Dir Option to Heading
	int tempDir = dirLogic[0];							
	unsigned tempDist = distArray[0];
	switch (heading)
	{
	case SOUTH:
		dirLogic[0] = dirLogic[1];
		dirLogic[1] = tempDir;
		distArray[0] = distArray[1];
		distArray[1] = tempDist;
		break;
	case EAST:
		dirLogic[0] = dirLogic[2];
		dirLogic[2] = tempDir;
		distArray[0] = distArray[2];
		distArray[2] = tempDist;
		break;
	case WEST:
		dirLogic[0] = dirLogic[3];
		dirLogic[3] = tempDir;
		distArray[0] = distArray[3];
		distArray[3] = tempDist;
		break;
	default:
		break;
	}
	// Swap FloodFill & Movement Options in Order from Least to Greatest
	for (int i = 0; i < 4; i++)								
	{
		for (int j = i + 1; j < 4; j++)
		{
			if (distArray[i] > distArray[j])
			{
				unsigned tempDist = distArray[j]; distArray[j] = distArray[i]; distArray[i] = tempDist;
				int tempDir = dirLogic[j]; dirLogic[j] = dirLogic[i]; dirLogic[i] = tempDir;
			}
		}
		// Manhattan Distance INT_MAX -> INVALID Mvt Direction Conversion
		if (distArray[i] == INT_MAX)						
			dirLogic[i] = INVALID;
	}
	// Adjust dirLogic for Walls/Bounds or Traps
	for (int i = 0, j = 4; i < j; i++)
		if (checkWall(x, y, adjustDirection(dirLogic[i])))
		{
			int del = i;
			for (; del < j - 1; del++)
				dirLogic[del] = dirLogic[del + 1];
			dirLogic[del] = INVALID; j--; i--;
		}
}

// Adjusts Mouse Heading and Initiates Movement
inline MouseMovement Algorithm::mouseMvt(int dirLogic[])
{
	/* Mouse Movement */
	// Move: Adjust Heading, return MouseMovement
	switch (dirLogic[0])				
	{
	case NORTH:
		return MoveForward;
	case WEST:
		heading = adjustDirection(WEST);
		return TurnCounterClockwise;
	case EAST:
		heading = adjustDirection(EAST);
		return TurnClockwise;
	case SOUTH:
		heading = adjustDirection(SOUTH);
		return TurnAround;
	default:							
		return Finish;		// Debug: INVALID Movement Input Options -> Finish
	}
}

// Application of Stack and FloodFill Analysis
inline void Algorithm::stackFloodFill(unsigned x, unsigned y)
{
	/* FloodFill Algorithm */
	// Push Current Cell on Stack
	if (!c_Process[x][y])
	{
		Coordinate data(x, y);
		c_Stack.push(data);
		c_Process[x][y] = true;
	}

	// FloodFill Loop
	while (!c_Stack.empty())										
	{
		// Pop Coordinate & Mark c_Process
		Coordinate curCell = c_Stack.top();
		c_Stack.pop();
		c_Process[curCell.x][curCell.y] = false;								

		// Do NOT Update Center/Start if mazeTarget->Center/Start
		if (checkCenter(curCell.x, curCell.y) && mazeTarget)					
			continue;
		if (checkStart(curCell.x, curCell.y) && !mazeTarget)	
			continue;

		// MinValue Initialization to INVALID Max Value
		unsigned minValue = INT_MAX;													
		
		// !Wall: Analyze Neighbor MinValue
		for (int dir = 0; dir < 4; dir++)												
		{
			switch (dir)	
			{
			case NORTH:
				if (curCell.y < MAZE_Y - 1 && !checkWall(curCell.x, curCell.y, dir))
					if (m_dist[curCell.x][curCell.y + 1] < minValue)
						minValue = m_dist[curCell.x][curCell.y + 1];
				else
					continue;
				break;
			case SOUTH:
				if (curCell.y > 0 && !checkWall(curCell.x, curCell.y, dir))
					if (m_dist[curCell.x][curCell.y - 1] < minValue)
						minValue = m_dist[curCell.x][curCell.y - 1];
				else
					continue;
				break;
			case EAST:
				if (curCell.x < MAZE_X - 1 && !checkWall(curCell.x, curCell.y, dir))
					if (m_dist[curCell.x + 1][curCell.y] < minValue)
						minValue = m_dist[curCell.x + 1][curCell.y];
				else
					continue;
				break;
			case WEST:
				if (curCell.x > 0 && !checkWall(curCell.x, curCell.y, dir))
					if (m_dist[curCell.x - 1][curCell.y] < minValue)
						minValue = m_dist[curCell.x - 1][curCell.y];
				else
					continue;
				break;
			default: continue;
			}	
		}

		// No MinValue || Current Cell = MinValue + 1
		if (minValue == INT_MAX || m_dist[curCell.x][curCell.y] == minValue + 1)	
			continue;

		// FloodFill Dynamic Recursion: Set Current Cell FloodFill to MinValue + 1 to Create Continuity
		m_dist[curCell.x][curCell.y] = minValue + 1;							

		// Push Open Neighbors of Current Cell on Stack -> Maze-Wide FloodFill Update
		if (curCell.y < MAZE_Y - 1 && !c_Process[curCell.x][curCell.y + 1] && !checkWall(curCell.x, curCell.y, NORTH))
		{
			Coordinate data(curCell.x, curCell.y + 1);
			c_Stack.push(data);
			c_Process[curCell.x][curCell.y + 1] = true;
		}
		if (curCell.y > 0 && !c_Process[curCell.x][curCell.y - 1] && !checkWall(curCell.x, curCell.y, SOUTH))
		{
			Coordinate data(curCell.x, curCell.y - 1);
			c_Stack.push(data);
			c_Process[curCell.x][curCell.y - 1] = true;
		}
		if (curCell.x < MAZE_X - 1 && !c_Process[curCell.x + 1][curCell.y] && !checkWall(curCell.x, curCell.y, EAST))
		{
			Coordinate data(curCell.x + 1, curCell.y);
			c_Stack.push(data);
			c_Process[curCell.x + 1][curCell.y] = true;
		}
		if (curCell.x > 0 && !c_Process[curCell.x - 1][curCell.y] && !checkWall(curCell.x, curCell.y, WEST))
		{
			Coordinate data(curCell.x - 1, curCell.y);
			c_Stack.push(data);
			c_Process[curCell.x - 1][curCell.y] = true;
		}
	}
}

// Returns Actual Direction of Dir relativeDir Direction w/ Respect to Heading
inline Dir Algorithm::adjustDirection(int relativeDir)
{	// EX) Mouse Heading = EAST, relativeDir = WEST, return NORTH
	switch (heading)
	{
	case NORTH:
		switch (relativeDir)
		{
		case NORTH:
			return heading;
		case SOUTH:
			return SOUTH;
		case EAST:
			return EAST;
		case WEST:
			return WEST;
		default:
			return INVALID;
		}
	case SOUTH:
		switch (relativeDir)
		{
		case NORTH:
			return heading;
		case SOUTH:
			return NORTH;
		case EAST:
			return WEST;
		case WEST:
			return EAST;
		default:
			return INVALID;
		}
	case EAST:
		switch (relativeDir)
		{
		case NORTH:
			return heading;
		case SOUTH:
			return WEST;
		case EAST:
			return SOUTH;
		case WEST:
			return NORTH;
		default:
			return INVALID;
		}
	case WEST:
		switch (relativeDir)
		{
		case NORTH:
			return heading;
		case SOUTH:
			return EAST;
		case EAST:
			return NORTH;
		case WEST:
			return SOUTH;
		default:
			return INVALID;
		}
	default:
		return INVALID;
	}
}

// Returns Heading Direction Adjustment Towards relativeDir Direction w/ Respect to Dir Heading
inline Dir Algorithm::adjustTarget(int relativeDir)
{	// EX) Mouse Heading = SOUTH, relativeDir = EAST, return WEST (Left Turn/Target->Adjust CounterClockwise)
	switch (heading)
	{
	case NORTH:
		switch (relativeDir)
		{
		case NORTH:
			return NORTH;
		case SOUTH:
			return SOUTH;
		case EAST:
			return EAST;
		case WEST:
			return WEST;
		default:
			return INVALID;
		}
	case SOUTH:
		switch (relativeDir)
		{
		case NORTH:
			return SOUTH;
		case SOUTH:
			return NORTH;
		case EAST:
			return WEST;
		case WEST:
			return EAST;
		default:
			return INVALID;
		}
	case EAST:
		switch (relativeDir)
		{
		case NORTH:
			return WEST;
		case SOUTH:
			return EAST;
		case EAST:
			return NORTH;
		case WEST:
			return SOUTH;
		default:
			return INVALID;
		}
	case WEST:
		switch (relativeDir)
		{
		case NORTH:
			return EAST;
		case SOUTH:
			return WEST;
		case EAST:
			return SOUTH;
		case WEST:
			return NORTH;
		default:
			return INVALID;
		}
	default:
		return INVALID;
	}
}

// Checks if Current Position == Maze Center
inline bool Algorithm::checkCenter(unsigned x, unsigned y)
{
	if ((x == MAZE_X / 2 || x == MAZE_X / 2 - 1) && (y == MAZE_Y / 2 || y == MAZE_Y / 2 - 1))
		return true;
	return false;
}

// Checks if Current Position == Maze Start
inline bool Algorithm::checkStart(unsigned x, unsigned y)
{
	if (x == 0 && y == 0)
		return true;
	return false;
}

// ClearScreen Method
#ifdef _MSC_VER  //  Microsoft Visual C++

#include <windows.h>

void clearScreen()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	DWORD dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
	COORD upperLeft = { 0, 0 };
	DWORD dwCharsWritten;
	FillConsoleOutputCharacter(hConsole, TCHAR(' '), dwConSize, upperLeft,
		&dwCharsWritten);
	SetConsoleCursorPosition(hConsole, upperLeft);
}

#else  // not Microsoft Visual C++, so assume UNIX interface

#include <cstring>

void clearScreen()  // will just write a newline in an Xcode output window
{
	static const char* term = getenv("TERM");
	if (term == nullptr || strcmp(term, "dumb") == 0)
		cout << endl;
	else
	{
		static const char* ESC_SEQ = "\x1B[";  // ANSI Terminal esc seq:  ESC [
		cout << ESC_SEQ << "2J" << ESC_SEQ << "H" << flush;
	}
}

#endif

/* Extraneous Code & Notes */

//for (int j = MAZE_Y - 1; j >= 0; j--)
//{
//	for (int i = 0; i < MAZE_X; i++)
//	{
//		cout << m_dist[i][j];
//		if (m_dist[i][j] < 10)
//			cout << "   ";
//		else if (m_dist[i][j] < 100)
//			cout << "  ";
//		else
//			cout << " ";
//	}
//	cout << endl;
//}
