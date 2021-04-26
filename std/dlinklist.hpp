#ifndef DOUBLE_LINKED_LIST_HPP
#define DOUBLE_LINKED_LIST_HPP

#include <stdio.h>
#include <stdlib.h>

#include "linkable.hpp"
#include "dtypes.h"

class DoublyLinkedList {

	protected:
		Linkable *head;
		Linkable *tail;
		Linkable *current;
		int count;

		void initList(Linkable *);

	public:
		DoublyLinkedList();

		void addLast(Linkable *);
		void addFirst(Linkable *);

		// insert into list at current
		void addBefore(Linkable *);
		void addAfter(Linkable *);

		// remove current from list
		// return pointer so caller can free memory
		Linkable *remove();

		// move to next or previous and return object
		Linkable *next();
		Linkable *previous();

		// move current to top of list
		void gotoStart();
		// move current to end of list
		void gotoEnd();

		// return the current object
		Linkable *getObject();

		int getCount();
		FLAG isEmpty() { return(getCount()<=0); }

		//List4 compatibility methods
		int numNodes() { return getCount(); }
		Linkable *first() { gotoStart(); return(getObject()); }
		Linkable *pop() { gotoEnd(); return(remove()); }
		Linkable *next(Linkable *){ return(next()); }		// ignore the passed arg
		void add(Linkable *l) { addLast(l); }
		void addBefore(Linkable *a, Linkable *b) { addBefore(b); }
		Linkable *remove(Linkable *) { return(remove()); }
};

#endif
