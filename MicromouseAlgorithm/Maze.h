#ifndef Maze_h
#define Maze_h

#include <string>

#include "BitVector256.h"
#include "MazeDefinitions.h"
#include "Dir.h"
#include "PathFinder.h"

class Maze {
protected:
	BitVector256 wallNS;
	BitVector256 wallEW;
	Dir heading;
	PathFinder *pathFinder;
	unsigned mouseX;
	unsigned mouseY;

	bool isOpen(unsigned x, unsigned y, Dir d) const;
	void setOpen(unsigned x, unsigned y, Dir d);

	void moveForward();
	void moveBackward();

	inline void turnClockwise() {
		heading = clockwise(heading);
	}

	inline void turnCounterClockwise() {
		heading = counterClockwise(heading);
	}

	inline void turnAround() {
		heading = opposite(heading);
	}

public:
	Maze(MazeDefinitions::MazeEncodingName name, PathFinder *pathFinder);

	inline bool wallInFront() const {
		return !isOpen(mouseX, mouseY, heading);
	}

	inline bool wallOnLeft() const {
		return !isOpen(mouseX, mouseY, counterClockwise(heading));
	}

	inline bool wallOnRight() const {
		return !isOpen(mouseX, mouseY, clockwise(heading));
	}

	/**
	* Start running the mouse through the maze.
	* Terminates when the PathFinder's nextMovement method returns MouseMovement::Finish.
	*/
	void start();

	/**
	* This function draws the maze using ASCII characters.
	*
	* Queries the underlying PathFinder for additional maze info
	* and incorporates it in the maze rendering.
	* @param infoLen: specifies the max characters of info to be drawn. If no info is supplied, blank spaces will be inserted.
	* @return string of rendered maze
	*/
	std::string draw(const size_t infoLen = 4) const;
};

#endif