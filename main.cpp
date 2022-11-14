#include <QCoreApplication>
#include <QFile>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlQuery>
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
	QSqlQuery query;

	if (!query.exec("CREATE TABLE words (id INTEGER PRIMARY KEY, part_of_speech TEXT)")) {
		qCritical() << "Could not create words table in SQLITE database.";
		return -1;
	}

	if (!query.exec("CREATE TABLE senses (id INTEGER PRIMARY KEY, word_id INTEGER, definition TEXT)")) {
		qCritical() << "Could not create senses table in SQLITE database.";
		return -1;
	}

	// Parse XML
	QXmlStreamReader reader;
	reader.addData(content);

	QVariantList words;
	QVariantList parts_of_speech;
	QVariantList sense_;

	QString lastTag;

	while (!reader.atEnd()) {
		reader.readNext();

		if (!reader.isStartElement())
			continue;

		QString tag = reader.name().toString();

		if (tag == "hw") {
			// head word (the word itself, acts as primary key)
			words << reader.text().toString();
		} else if (tag == "sn") {
			// expect a sense
			query.prepare("INSERT INTO senses VALUES (:id, :word_id, :def)");
		}
	}

	// Add data to database
	query.prepare("INSERT INTO words VALUES (:id, :pos)");
	query.bindValue(":id", words);
	query.bindValue(":pos", parts_of_speech);

	if (!query.exec()) {
		qCritical() << "Could not add words to SQLITE database.";
		return -1;
	}

	return a.exec();
}
