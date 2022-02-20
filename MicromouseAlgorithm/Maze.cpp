#include <iostream>
#include "Maze.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))

Maze::Maze(MazeDefinitions::MazeEncodingName name, PathFinder *pathFinder)
	: heading(NORTH), pathFinder(pathFinder), mouseX(0), mouseY(0) {
	if (name >= MazeDefinitions::MAZE_NAME_MAX) {
		name = MazeDefinitions::MAZE_CAMM_2012;
	}

	const unsigned mazeIndex = ((unsigned)name < ARRAY_SIZE(MazeDefinitions::mazes)) ? (unsigned)name : 0;

	wallNS.clearAll();
	wallEW.clearAll();

	// Encoding stores wall/no wall in WSEN in the least significant bits
	// Data is stored in column major order
	const unsigned westMask = 1 << 3;
	const unsigned southMask = 1 << 2;
	const unsigned eastMask = 1 << 1;
	const unsigned northMask = 1 << 0;

	for (unsigned col = 0; col < MazeDefinitions::MAZE_LEN; col++) {
		for (unsigned row = 0; row < MazeDefinitions::MAZE_LEN; row++) {
			const unsigned char cell = MazeDefinitions::mazes[mazeIndex][col][row];

			if ((cell & northMask) == 0 && row != MazeDefinitions::MAZE_LEN) {
				setOpen(col, row, NORTH);
			}

			if ((cell & southMask) == 0 && row != 0) {
				setOpen(col, row, SOUTH);
			}

			if ((cell & westMask) == 0 && col != 0) {
				setOpen(col, row, WEST);
			}

			if ((cell & eastMask) == 0 && col != MazeDefinitions::MAZE_LEN) {
				setOpen(col, row, EAST);
			}
		}
	}
}

bool Maze::isOpen(unsigned x, unsigned y, Dir d) const {
	switch (d) {
	case NORTH:
		return wallNS.get(x, y + 1);
	case SOUTH:
		return wallNS.get(x, y);
	case EAST:
		return wallEW.get(x + 1, y);
	case WEST:
		return wallEW.get(x, y);
	case INVALID:
	default:
		return false;
	}
}

void Maze::setOpen(unsigned x, unsigned y, Dir d) {
	switch (d) {
	case NORTH:
		return wallNS.set(x, y + 1);
	case SOUTH:
		return wallNS.set(x, y);
	case EAST:
		return wallEW.set(x + 1, y);
	case WEST:
		return wallEW.set(x, y);
	case INVALID:
	default:
		return;
	}
}

void Maze::moveForward() {
	if (!isOpen(mouseX, mouseY, heading)) {
		throw "Mouse crashed!";
	}

	switch (heading) {
	case NORTH:
		mouseY++;
		break;
	case SOUTH:
		mouseY--;
		break;
	case EAST:
		mouseX++;
		break;
	case WEST:
		mouseX--;
		break;
	case INVALID:
	default:
		break;
	}
}

void Maze::moveBackward() {
	Dir oldHeading = heading;
	heading = opposite(heading);
	moveForward();
	heading = oldHeading;
}

void Maze::start() {
	MouseMovement nextMovement;

	if (!pathFinder) {
		return;
	}

	//Maze Data passed to PathFinder nextMovement
	while (Finish != (nextMovement = pathFinder->nextMovement(mouseX, mouseY, *this))) {
		try {
			switch (nextMovement) {
			case MoveForward:
				moveForward();
				break;
			case MoveBackward:
				moveBackward();
				break;
			case TurnClockwise:
				turnClockwise();
				break;
			case TurnCounterClockwise:
				turnCounterClockwise();
				break;
			case TurnAround:
				turnAround();
				break;
			case Finish:
			default:
				return;
			}
		}
		catch (std::string str) {
			std::cerr << str << std::endl;
		}
	}
}

std::string Maze::draw(const size_t infoLen) const {
	std::string out("");
	std::string upDown, leftRight;

	const size_t cellWidth = infoLen + 1;
	const char dot = '*';
	const char vertWall = '|';
	const char vertWallEmpty = ' ';
	const std::string horizWall = std::string("").append(cellWidth, '-');
	const std::string horizWallEmpty = std::string("").append(cellWidth, ' ');

	for (unsigned row = 0; row < MazeDefinitions::MAZE_LEN; row++) {
		const unsigned y = MazeDefinitions::MAZE_LEN - row - 1;

		upDown = dot;
		leftRight = "";

		// Draw most of the maze
		for (unsigned x = 0; x < MazeDefinitions::MAZE_LEN; x++) {
			std::string cellInfo;

			if (pathFinder) {
				cellInfo = pathFinder->getInfo(x, y, infoLen).substr(0, infoLen);
			}

			if ("" == cellInfo) {
				cellInfo.append(cellWidth / 2, vertWallEmpty);
			}

			if (x == mouseX && y == mouseY) {
				switch (heading) {
				case NORTH:
					cellInfo += '^';
					break;
				case SOUTH:
					cellInfo += 'V';
					break;
				case EAST:
					cellInfo += '>';
					break;
				case WEST:
					cellInfo += '<';
					break;
				case INVALID:
				default:
					break;
				}
			}

			if (cellInfo.length() < cellWidth) {
				cellInfo.append(cellWidth - cellInfo.length(), ' ');
			}

			upDown += isOpen(x, y, NORTH) ? horizWallEmpty : horizWall;
			leftRight += isOpen(x, y, WEST) ? vertWallEmpty : vertWall;

			upDown += dot;
			leftRight += cellInfo;
		}

		// Get the last column of walls
		leftRight += isOpen(MazeDefinitions::MAZE_LEN - 1, y, EAST) ? vertWallEmpty : vertWall;

		out += upDown + '\n' + leftRight + '\n';
	}

	// Draw out the bottom most row
	out += dot;
	for (unsigned x = 0; x < MazeDefinitions::MAZE_LEN; x++) {
		out += isOpen(x, 0, SOUTH) ? horizWallEmpty : horizWall;
		out += dot;
	}

	return out;
}