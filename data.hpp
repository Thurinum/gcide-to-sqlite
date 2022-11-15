#ifndef DATA_HPP
#define DATA_HPP

#include <QList>

struct Sense
{
	int	  id;
	int	  word_id;
	int	  definition_number;
	QString definition;
};

struct Word
{
	int	  id;
	QString entry;
	QString part_of_speech;
};

#endif // DATA_HPP
