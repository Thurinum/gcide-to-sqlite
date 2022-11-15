#include <QCoreApplication>
#include <QDomDocument>
#include <QFile>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "data.hpp"

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	// Load test file
	QFile file("gcide-0.53/TEMP.SAMPLE");

	if (!file.open(QIODevice::ReadOnly)) {
		qCritical() << "Could not open dictionary file.";
		return -1;
	}

	// Sanitize content to be passed to XML parser
	QString content = file.readAll();
	content.replace(QRegularExpression("<([a-zA-Z]+)\/"), "<\\1 \/>");
	content.replace(QRegularExpression("<--"), "<!--");
	content.replace(QRegularExpression("&"), "&amp;");
	content.prepend("<dictionary>");
	content.append("</dictionary>");

	// Setup SQLITE database
	QFile::remove("gcide.sqlite");
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName("gcide.sqlite");

	if (!db.open()) {
		qCritical() << "Could not connect to SQLITE db.";
		return -1;
	}

	// Create table
	QSqlQuery query;

	if (!query.exec("CREATE TABLE words (id INTEGER PRIMARY KEY, entry TEXT, part_of_speech "
			    "TEXT)")) {
		qCritical() << "Could not create words table in SQLITE database.";
		return -1;
	}

	if (!query.exec("CREATE TABLE senses (id INTEGER PRIMARY KEY, definition_number "
			    "INTEGER, word_id INTEGER, definition TEXT)")) {
		qCritical() << "Could not create senses table in SQLITE database.";
		return -1;
	}

	// Parse XML
	QDomDocument doc;
	doc.setContent(content);

	QList<Word*>  words;
	QList<Sense*> senses;

	bool hasSenseTag	 = false;
	int  currentWordId = 0;

	QDomNodeList paragraphs = doc.documentElement().elementsByTagName("p");

	for (int i = 0; i < paragraphs.length(); i++) {
		QDomNode	p    = paragraphs.at(i);
		QDomElement word = p.firstChildElement("hw");

		if (!word.isNull()) {
			// it's a new word
			words.append(new Word());
			words.last()->id		     = currentWordId;
			words.last()->entry	     = word.text();
			words.last()->part_of_speech = p.firstChildElement("pos").text();
			currentWordId++;

			QDomElement sense = p.firstChildElement("sn");
			if (!sense.isNull()) {
				// word has more than one sense and is explicitly marked with <sn>
				senses.append(new Sense());
				senses.last()->word_id		   = words.last()->id;
				senses.last()->definition_number = sense.text().first(0).toInt();
				senses.last()->definition	   = p.firstChildElement("def").text();
			}
		} else {
			QDomElement sense = p.firstChildElement("sn");

			if (!sense.isNull()) {
				// it's a sense of the current word
				senses.append(new Sense());
				senses.last()->word_id		   = words.last()->id;
				senses.last()->definition_number = sense.text().first(0).toInt();
				senses.last()->definition	   = p.firstChildElement("def").text();
			} else {
				// it's something else... note, etc.
			}
		}
	}

	qDebug() << words.length() << " words";
	qDebug() << senses.length() << " senses";

	for (Word* w : words)
		qDebug() << w->id << w->entry << w->part_of_speech;

	for (Sense* s : senses)
		qDebug() << s->id << s->word_id << s->definition << s->definition_number;

	return a.exec();
}
