#ifndef DATA_HPP
#define DATA_HPP

#include <QList>

class Sense
{
	int	  id;
	int	  num;
	QString definition;
};

struct Word
{
	QString	 id;
	QList<Sense> senses;
};

#endif // DATA_HPP
