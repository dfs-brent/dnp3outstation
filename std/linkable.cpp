#include "linkable.hpp"

Linkable::Linkable()
{
	prev = NULL;
	next = NULL;
}
Linkable *Linkable::getNext()
{
	return(next);
}
Linkable *Linkable::getPrevious()
{
	return(prev);
}
void Linkable::setNext(Linkable *l)
{
	next = l;
}
void Linkable::setPrevious(Linkable *l)
{
	prev = l;
}
