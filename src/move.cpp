#ifndef MOVE_CPP
#define MOVE_CPP

#include "move.h"

void Move::clear()
{
	move = 0;
}    

void Move::setFrom(int from)  
{  
	move &= 0xffffffc0; 
	move |= from;
}
 
void Move::setTo(int to)  
{  
	move &= 0xfffff03f; 
	move |= to << 6;
}
 
void Move::setPromotion(int promotion)
{  
	move &= 0xffff0fff;
	move |= promotion << 12;
}

void Move::setScore(int s)
{
	score = s;
}

void Move::setMove(int m)
{
	move = m;
}

int Move::getMove()
{
	return move;
}

int Move::getFrom()  
{ 
	return (move & 0x0000003f);
}  
 
int Move::getTo()  
{  
	return (move >>  6) & 0x0000003f; 
}   
 
int Move::getPromotion()  
{ 
	return (move >> 12) & 0x0000000f; 
}   

int Move::getScore()
{
	return score;
}
 
#endif