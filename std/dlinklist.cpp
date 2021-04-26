#include "dlinklist.hpp"

DoublyLinkedList::DoublyLinkedList()
{
	head = NULL;
	tail = NULL;
	current = NULL;
}
void DoublyLinkedList::initList(Linkable *obj)
{
	// first object
	tail = obj;
	head = obj;
	current = obj;
	obj->setNext(NULL);
	obj->setPrevious(NULL);
	count = 1;
}
// always add to end.  use insert otherwise
void DoublyLinkedList::addLast(Linkable *obj)
{
	gotoEnd();
	addAfter(obj);
}

// use add to add the first object, this will
// only insert a record if current is not null
void DoublyLinkedList::addFirst(Linkable *obj)
{
	gotoStart();
	addBefore(obj);
}

// insert into list before the current
void DoublyLinkedList::addBefore(Linkable *obj)
{
	if(current == NULL)
	{
		initList(obj);
	}
	else
	{
		obj->setNext(current);
		obj->setPrevious(current->getPrevious());
		if(obj->getPrevious() != NULL)
			(obj->getPrevious())->setNext(obj);
		else
		{
			// adding to beginning of list
			head = obj;
		}
		current->setPrevious(obj);
		current = obj;
		count++;
	}
}
// add obj after current and move current to obj
// also adjust tail if this is the end of the list
void DoublyLinkedList::addAfter(Linkable *obj)
{
	if(current == NULL)
	{
		initList(obj);
	}
	else
	{
		obj->setNext(current->getNext());
		obj->setPrevious(current);
		if(obj->getNext() != NULL)
			(obj->getNext())->setPrevious(obj);
		else
		{
			// adding to tail
			tail = obj;
		}
		current->setNext(obj);
		current = obj;
		count++;
	}
}

// remove current from list
// return pointer so caller can free memory
// move current FORWARD one spot if removed
// object was not the tail.  Otherwise move
// backward one spot.
Linkable *DoublyLinkedList::remove()
{
	Linkable *tmp = current;
	if(current == NULL)
	{
		// empty list or bad error here
		count = 0;
		return((Linkable *)NULL);
	}
	if(current == head)
	{
		// move it forward
		current = current->getNext();
		if(current != NULL)
			current->setPrevious(NULL);
		else
			// empty list
			tail = current;
		// reset head of the list
		head = current;
	}
	else if(current == tail)
	{
		current = current->getPrevious();
		if(current != NULL)
			current->setNext(NULL);
		else
			// empty list
			head = current;
		// reset the tail
		tail = current;
	}
	else
	{
		// we're in the middle somewhere
		(current->getPrevious())->setNext(current->getNext());
		(current->getNext())->setPrevious(current->getPrevious());
		// move current forward
		current = current->getNext();
	}
	count--;
	return(tmp);
}

// move to next or previous and return object
Linkable *DoublyLinkedList::next()
{
	if(current != NULL)
	{
		// dont move past the end
		if(current->getNext() == NULL)
			return((Linkable *)NULL);
		current = current->getNext();
	}
	return(current);
}
Linkable *DoublyLinkedList::previous()
{
	if(current != NULL)
	{
		// dont go past the start
		if(current->getPrevious() == NULL)
			return((Linkable *)NULL);
		current = current->getPrevious();
	}
	return(current);
}

// move current to top of list
void DoublyLinkedList::gotoStart()
{
	current = head;
}
// move current to end of list
void DoublyLinkedList::gotoEnd()
{
	current = tail;
}
// return the current object
Linkable *DoublyLinkedList::getObject()
{
	return(current);
}

int DoublyLinkedList::getCount()
{
	return(count);
}

