#ifndef LINKABLE_HPP
#define LINKABLE_HPP

#include <stdio.h>
#include <stdlib.h>


class Linkable {

	private:
		Linkable *prev;
		Linkable *next;

	public:
		Linkable();

		Linkable *getNext();
		Linkable *getPrevious();
		void setNext(Linkable *);
		void setPrevious(Linkable *);
};

#endif
