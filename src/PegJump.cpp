/*
 * PegJump.cpp
 *
 *  Created on: Feb 5, 2013
 *      Author: kevin
 */

#include <iostream>
#include <stdlib.h>
#include <set>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

using std::set;
using std::cout;
using std::endl;
using std::cin;
using std::cerr;

#define TOP_PEG_Y	120 // pixels
#define ROW_SPACING	74 // pixels
#define COL_SPACING 84
#define CENTER_COL	320

#define PLAY_ON 	0
#define WIN 		1
#define LOSE 		2
#define EXIT		3

const int SCREEN_HEIGHT = 480;
const int SCREEN_WIDTH = 640;
const int SCREEN_BPP = 32;

SDL_Surface *screen = NULL;
SDL_Surface *peg = NULL;
SDL_Surface *background = NULL;
SDL_Surface *curBoardImg = NULL;
SDL_Event event;

/* Position Numbers
 *             0,0
 *          1,0   1,1
 *       2,0   2,1   2,2
 *    3,0   3,1   3,2   3,3
 * 4,0   4,1   4,2   4,3   4,4
 */
class Position {
	int myRow, myOffset;
	int imgX, imgY;
public:
	Position(void);
	Position(int a, int b);
	int row(void) const {return myRow;}
	int offset(void) const {return myOffset;}
	int getImgX(void) const {return imgX; }
	int getImgY(void) const {return imgY; }
	void calcImageOffset(void);
	void populateMovesList(void);
	void printMoveList(void) const;
	Position operator-(const Position &other) const {
		return Position(myRow - other.row(), myOffset - other.offset());
	}
	Position operator+(const Position &other) const {
		return Position(myRow + other.row(), myOffset + other.offset());
	}
	Position operator/(const int a) const {
		return Position(myRow / a, myOffset / a);
	}

	struct compare {
		bool operator() (const Position &a, const Position &b) const
		{
			return ((a.myRow*50 + a.myOffset) < (b.myRow*50 + b.myOffset));
		}
	};

	set<Position, Position::compare> posMoves;
};
Position::Position() {
	this->myRow = -1;
	this->myOffset = -1;
	this->imgX = -1;
	this->imgY = -1;
}
Position::Position(int a, int b) {
	this->myRow = a;
	this->myOffset = b;
	this->calcImageOffset();
}

void Position::calcImageOffset() {
	// this function is heavily dependent on board image (obviously)
	// returns offset of peg placed on hole, ie position of top left of peg image
	this->imgY = TOP_PEG_Y + myRow * ROW_SPACING - (peg->h)/2;
	this->imgX = CENTER_COL + (myRow % 2) * (COL_SPACING/2) * -1 +
			(myOffset - myRow/2) * COL_SPACING - (peg->w)/2;
}

void Position::populateMovesList() {
	for (int i = 0; i <= 4; i++) {
		for (int j = 0; j <=i; j++) {
			if (	!(myRow == i && myOffset == j) &&
					(myOffset == j || myOffset == j-2 || myOffset == j+2) &&
					(myRow == i || myRow == i-2 || myRow == i+2)
					) {
				if ((this->row() == 2 && this->offset()==2 && i == 4 && j == 0) ||
						(this->row() == 4 && this->offset()==0 && i == 2 && j == 2)) // exception case to the rules
					continue;
				posMoves.insert(Position(i, j));
			}
		}
	}
}
void Position::printMoveList() const {
	for (set<Position>::iterator it = this->posMoves.begin(); it != this->posMoves.end(); it++) {
		cout<<'\t'<<it->row()<<", "<<it->offset()<<endl;
	}
}

bool legalMove(std::set<Position, Position::compare> pegList, Position from, Position to) {
	// first check moves list
	set<Position>::iterator it;
	if (((it = pegList.find(from)) != pegList.end()) && (pegList.find(to) == pegList.end())) {
		if (it->posMoves.find(to) != it->posMoves.end()) { // can you actually jump there?
			if (pegList.find((from - to)/2 + to) != pegList.end()) { // something to jump over
				return true;
			}
		}
	}
	return false;
}

bool movePeg(std::set<Position, Position::compare> &pegList, Position start, Position end) {
	if (!legalMove(pegList, start, end))
		return false;
	pegList.erase(start);
	pegList.erase((start - end)/2 + end);
	Position newPos = Position(end.row(), end.offset());
	newPos.populateMovesList();
	pegList.insert(newPos);
	return true;
}

int gameOver(std::set<Position, Position::compare> pegList) {
	if (pegList.size() < 2)
		return WIN;
	if (pegList.size() > 6)
		return PLAY_ON;

	// Now check the hard way
	for (set<Position>::iterator pegIt = pegList.begin(); pegIt != pegList.end(); pegIt++) {
		for (set<Position>::iterator moveIt = pegIt->posMoves.begin(); moveIt != pegIt->posMoves.end(); moveIt++) {
			if (pegList.find(*moveIt) == pegList.end()) { // spot available
				Position jumpOver = Position((moveIt->row() - pegIt->row())/2 + pegIt->row(),
						(moveIt->offset() - pegIt->offset())/2 + pegIt->offset());
				if (pegList.find(jumpOver) != pegList.end()) // and a peg to jump over
					return PLAY_ON;
			}
		}
	}
	return LOSE;
}

void printBoard(set<Position, Position::compare> pegList) {
	int present[15];
	int k = 0;
	set<Position, Position::compare>::iterator it = pegList.begin();
	for (int i = 0; i<=4; i++) {
		for (int j = 0; j<=i; j++) {
			if ((it->row() == i) && (it->offset() == j)) {
				present[k++] = 1;
				it++;
			}
			else
				present[k++] = 0;
		}
	}
	cout<<"            "<<present[0]<<endl;
	cout<<"           "<<present[1]<<" "<<present[2]<<endl;
	cout<<"          "<<present[3]<<" "<<present[4]<<" "<<present[5]<<endl;
	cout<<"         "<<present[6]<<" "<<present[7]<<" "<<present[8]<<" "<<present[9]<<endl;
	cout<<"        "<<present[10]<<" "<<present[11]<<" "<<present[12]<<" "<<present[13]<<" "<<present[14]<<endl;
}

void apply_surface(int x, int y, SDL_Surface *source, SDL_Surface *destination, SDL_Rect *clip = NULL) {
	SDL_Rect offset;
	offset.x = x;
	offset.y = y;
	SDL_BlitSurface(source, clip, destination, &offset);
}

bool init() {
	if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
		return false;
	}
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_SWSURFACE);
	if (screen == NULL) {
		return false;
	}
	SDL_WM_SetCaption("Peg Jump", NULL);
	return true;
}

SDL_Surface *load_image(std::string filename) {
	SDL_Surface *loadedImage = NULL;
	SDL_Surface *optimizedImage = NULL;
	loadedImage = IMG_Load(filename.c_str());
	if (loadedImage != NULL) {
		optimizedImage = SDL_DisplayFormat(loadedImage);
	}
	SDL_FreeSurface(loadedImage);
	return optimizedImage;
}

void clean_up() {
	SDL_FreeSurface(background);
	SDL_FreeSurface(peg);
	SDL_FreeSurface(curBoardImg);
	SDL_Quit();
}

bool pointerInsidePeg(set<Position, Position::compare> pegset, int x, int y, Position *src) {
	set<Position, Position::compare>::iterator it;
	for (it = pegset.begin();  it != pegset.end(); it++) {
		if ((event.button.x > it->getImgX()) &&
				(event.button.x < it->getImgX() + peg->w) &&
				(event.button.y > it->getImgY()) &&
				(event.button.y < it->getImgY() + peg->h)) {
			*src = *it;
			return true;
		}
	}
	return false;
}

bool pointerInsideLegalMove(set<Position, Position::compare> pegset, int x, int y, Position *src, Position *dest) {
	set<Position, Position::compare>::iterator moveIt;
	Position pos;
	for (moveIt = src->posMoves.begin(); moveIt != src->posMoves.end(); moveIt++) {
		pos = Position(moveIt->row(), moveIt->offset());
		if ((x > pos.getImgX()) &&
				(x < pos.getImgX() + peg->w) &&
				(y > pos.getImgY()) &&
				(y < pos.getImgY() + peg->h) &&
				legalMove(pegset, *src, pos)) {
			*dest = pos;
			return true;
		}
	}
	dest = NULL;
	return false;
}

void updateCurBoard(set<Position, Position::compare> pegset, Position pos = Position()) {
	curBoardImg = SDL_ConvertSurface(background, background->format, background->flags);
	for (set<Position, Position::compare>::iterator it = pegset.begin();
			it != pegset.end();
			it++) {
		if ((it->row() != pos.row()) || (it->offset() != pos.offset())) {
			apply_surface(it->getImgX(), it->getImgY(), peg, curBoardImg);
		}
	}
}

int main()
{
	Uint32 mouseState;
	int mouseX, mouseY;
	int deltaX, deltaY; // offsets of pointer and peg location
	set<Position>::iterator it;
	Position srcPos, destPos;
	set<Position, Position::compare> curPegs;
	int status;

	if (init() == false) {
		return 1;
	}
	background = load_image("board.bmp");
	if (background == NULL) {
		cerr<<"Could not open board.bmp"<<endl;
		return 1;
	}
	apply_surface(0, 0, background, screen);
	peg = load_image("peg.bmp");
	if (peg != NULL) {
		Uint32 colorkey = SDL_MapRGB(peg->format, 0xff, 0xff, 0xff);
		SDL_SetColorKey(peg, SDL_SRCCOLORKEY, colorkey);
	}
	else {
		cerr<<"Could not open peg.bmp"<<endl;
		return 1;
	}

	// create and populate list of current pegs
	for (int i = 1; i <= 4; i++) {
		for (int j = 0; j <= i; j++) {
			Position pos = Position(i, j);
			pos.populateMovesList();
			curPegs.insert(pos);
			apply_surface(pos.getImgX(), pos.getImgY(), peg, screen);
		}
	}
	if (SDL_Flip(screen) == -1) {
		cerr<<"Failed to update screen"<<endl;
		return 1;
	}
	status = PLAY_ON;
	while (status == PLAY_ON && !(status = gameOver(curPegs))) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_MOUSEBUTTONDOWN &&
					event.button.button == SDL_BUTTON_LEFT) {
				// check if the click is in a peg
				if (pointerInsidePeg(curPegs, event.button.x, event.button.y, &srcPos)) {
					deltaX = event.button.x - srcPos.getImgX();
					deltaY = event.button.y - srcPos.getImgY();
					updateCurBoard(curPegs, srcPos);
					do {
						SDL_PumpEvents();
						mouseState = SDL_GetMouseState(&mouseX, &mouseY);
						apply_surface(0, 0, curBoardImg, screen);
						apply_surface(mouseX - deltaX, mouseY - deltaY, peg, screen);
						if (SDL_Flip(screen) == -1) {
							cout<<"Failed to update screen"<<endl;
							return 1;
						}
					} while (mouseState & SDL_BUTTON(1));
					// check if user let go on a legal space
					if (pointerInsideLegalMove(curPegs, mouseX, mouseY, &srcPos, &destPos)) {
						movePeg(curPegs, srcPos, destPos);
					}
					updateCurBoard(curPegs);
					apply_surface(0, 0, curBoardImg, screen);
					if (SDL_Flip(screen) == -1) {
						cout<<"Failed to update screen"<<endl;
						return 1;
					}
				}
			}
			else if (event.type == SDL_QUIT) {
				status = EXIT;
			}
		}
	}
	if (status != EXIT) {
		SDL_Delay(2000);
	}
	clean_up();
	return 0;
}
