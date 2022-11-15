#include <QCoreApplication>
#include <QFile>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QXmlStreamReader>

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
	QSqlQuery wordQuery;
	QSqlQuery senseQuery;

	if (!wordQuery.exec("CREATE TABLE words (id INTEGER PRIMARY KEY, entry TEXT, part_of_speech "
				  "TEXT)")) {
		qCritical() << "Could not create words table in SQLITE database.";
		return -1;
	}

	if (!senseQuery.exec("CREATE TABLE senses (id INTEGER PRIMARY KEY, definition_number "
				   "INTEGER, word_id INTEGER, definition TEXT)")) {
		qCritical() << "Could not create senses table in SQLITE database.";
		return -1;
	}

	// Parse XML
	QXmlStreamReader reader;
	reader.addData(content);

	int wordCount = 0;
	bool hasSense = false;

	while (!reader.atEnd()) {
		reader.readNext();

		if (!reader.isStartElement())
			continue;

		QString tag = reader.name().toString();

		if (tag == "hw") {
			if (wordCount != 0 && !wordQuery.exec())
				qCritical() << "Failed to add word to database.";

			wordCount++;
			hasSense = false;
			wordQuery.prepare("INSERT INTO words VALUES (NULL, :entry, :pos)");
			wordQuery.bindValue(":entry", reader.readElementText());
		} else if (tag == "pos") {
			wordQuery.bindValue(":pos", reader.readElementText());
		} else if (tag == "def") {
			if (!hasSense) {
				senseQuery.prepare("INSERT INTO senses VALUES (NULL, 0, :word_id, :def)");
				senseQuery.bindValue(":word_id", wordCount);
			}
			senseQuery.bindValue(":def", reader.readElementText());

			if (!senseQuery.exec())
				qCritical()
					<< "Could not add sense to database: " << senseQuery.lastError();
		} else if (tag == "sn") {
			senseQuery.prepare("INSERT INTO senses VALUES (NULL, :num, :word_id, :def)");
			senseQuery.bindValue(":num", reader.readElementText().toInt());
			hasSense = true;
		}
	}

	QSqlQuery test;
	test.exec("SELECT * FROM words");

	test.seek(0);
	qDebug() << test.value(0);

	return a.exec();
}
