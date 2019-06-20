#include "database.h"
#include <QSqlDatabase>
#include <QFile>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QVariant>

Database::Database(QObject *parent) : QObject(parent)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

    if (!QFile("AviacominfoTools.db").exists())
        qDebug() << "File database is not found.";

    db.setDatabaseName("AviacominfoTools.db");

    if (!db.open())
        qDebug() << "Can not connected to database.";
}

void Database::update(Airfield airfield, QVector<QVector<QString> > &obstracles)
{
    QSqlQuery query;

    QSqlDatabase::database().transaction();

    query.prepare("INSERT INTO airfield (name, code_icao) SELECT :name, :code_icao WHERE NOT EXISTS(SELECT 1 "
                  "FROM airfield WHERE name = :name AND code_icao = :code_icao)");
    query.bindValue(":name", airfield.name);
    query.bindValue(":code_icao", airfield.icao);
    if (!query.exec()) {
        qDebug() << query.lastError().text();
        qDebug() << query.lastQuery();
        qDebug() << query.boundValues();
        return;
    }

    // get id airfield
    query.prepare("SELECT id FROM airfield WHERE name = ? AND code_icao = ?");
    query.addBindValue(airfield.name);
    query.addBindValue(airfield.icao);
    if (!query.exec()) {
        qDebug() << query.lastError().text() << " " << query.lastQuery();
    }
    query.first();
    uint idAirfield = query.value(0).toUInt();

    for (int i = 0; i < obstracles.size(); i++) {
        query.prepare("INSERT INTO coordinate_system (name) SELECT :name WHERE NOT EXISTS(SELECT 1 "
                      "FROM coordinate_system WHERE name = :name)");
        query.bindValue(":name", obstracles.at(i).at(4));
        if (!query.exec()) {
            qDebug() << query.lastError().text();
            qDebug() << query.lastQuery();
            qDebug() << query.boundValues();
        }
        else {
            // get id coordination system
            query.prepare("SELECT id FROM coordinate_system WHERE name = ?");
            query.addBindValue(obstracles.at(i).at(4));
            if (!query.exec()) {
                qDebug() << query.lastError().text() << " " << query.lastQuery();
            }
        }
        QVariant idCoordinationSystem = query.first() ? query.value(0).toUInt() : QVariant(QVariant::UInt);

        query.prepare("INSERT INTO fragility (name) SELECT :name WHERE NOT EXISTS(SELECT 1 \
                      FROM fragility WHERE name = :name)");
        query.bindValue(":name", obstracles.at(i).at(15));
        if (!query.exec()) {
            qDebug() << query.lastError().text() << " " << query.lastQuery();
        }
        else {
            // get id fragility
            query.prepare("SELECT id FROM fragility WHERE name = ?");
            query.addBindValue(obstracles.at(i).at(15));
            if (!query.exec()) {
                qDebug() << query.lastError().text() << " " << query.lastQuery();
            }
        }
        QVariant idFragility = query.first() ? query.value(0).toUInt() : QVariant(QVariant::UInt);

        query.prepare("INSERT INTO locality (name) SELECT :name WHERE NOT EXISTS(SELECT 1 \
                      FROM locality WHERE name = :name)");
        query.bindValue(":name", obstracles.at(i).at(3));
        if (!query.exec()) {
            qDebug() << query.lastError().text() << " " << query.lastQuery();
        }
        else {
            // get id locality
            query.prepare("SELECT id FROM locality WHERE name = ?");
            query.addBindValue(obstracles.at(i).at(3));
            if (!query.exec()) {
                qDebug() << query.lastError().text() << " " << query.lastQuery();
            }
        }
        QVariant idLocality = query.first() ? query.value(0).toUInt() : QVariant(QVariant::UInt);

        query.prepare("INSERT INTO type_configuration_obstracle (name) SELECT :name WHERE NOT EXISTS(SELECT 1 \
                      FROM type_configuration_obstracle WHERE name = :name)");
        query.bindValue(":name",  obstracles.at(i).at(2));
        if (!query.exec()) {
            qDebug() << query.lastError().text() << " " << query.lastQuery();
        }
        else {
            // get id type configuration obstracle
            query.prepare("SELECT id FROM type_configuration_obstracle WHERE name = ?");
            query.addBindValue(obstracles.at(i).at(2));
            if (!query.exec()) {
                qDebug() << query.lastError().text() << " " << query.lastQuery();
            }
        }
        QVariant idTypeConfigurationObstracle = query.first() ? query.value(0).toUInt() : QVariant(QVariant::UInt);

        query.prepare("INSERT INTO type_material (name) SELECT :name WHERE NOT EXISTS(SELECT 1 \
                      FROM type_material WHERE name = :name)");
        query.bindValue(":name", obstracles.at(i).at(14));
        if (!query.exec()) {
            qDebug() << query.lastError().text() << " " << query.lastQuery();
        }
        else {
            // get id type material
            query.prepare("SELECT id FROM type_material WHERE name = ?");
            query.addBindValue(obstracles.at(i).at(14));
            if (!query.exec()) {
                qDebug() << query.lastError().text() << " " << query.lastQuery();
            }
        }
        QVariant idTypeMaterial = query.first() ? query.value(0).toUInt() : QVariant(QVariant::UInt);

        query.prepare("INSERT OR REPLACE INTO obstracle (id, name, type_configuration, locality, coordinate_system, \
                        latitude, longitude, latitude_center, longitude_center, radius, horizontal_accuracy, \
                        orthometric_height, relative_height, vertical_precision, type_material, fragility, \
                        marking_daytime, marking_daytime_template, marking_daytime_color, night_marking, \
                        night_marking_color, night_marking_type_light, night_marking_intensity, \
                        night_marking_work_time, accordance_icao, source_data, date_updated, airfield) \
                        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        query.addBindValue(obstracles.at(i).at(0));
        query.addBindValue(obstracles.at(i).at(1));
        query.addBindValue(idTypeConfigurationObstracle);
        query.addBindValue(idLocality);
        query.addBindValue(idCoordinationSystem);
        query.addBindValue(obstracles.at(i).at(5));
        query.addBindValue(obstracles.at(i).at(6));
        query.addBindValue(obstracles.at(i).at(7));
        query.addBindValue(obstracles.at(i).at(8));
        query.addBindValue(obstracles.at(i).at(9));
        query.addBindValue(obstracles.at(i).at(10));
        query.addBindValue(obstracles.at(i).at(11));
        query.addBindValue(obstracles.at(i).at(12));
        query.addBindValue(obstracles.at(i).at(13));
        query.addBindValue(idTypeMaterial);
        query.addBindValue(idFragility);
        query.addBindValue(obstracles.at(i).at(16));
        query.addBindValue(obstracles.at(i).at(17));
        query.addBindValue(obstracles.at(i).at(18));
        query.addBindValue(obstracles.at(i).at(19));
        query.addBindValue(obstracles.at(i).at(20));
        query.addBindValue(obstracles.at(i).at(21));
        query.addBindValue(obstracles.at(i).at(22));
        query.addBindValue(obstracles.at(i).at(23));
        query.addBindValue(obstracles.at(i).at(24));
        query.addBindValue(obstracles.at(i).at(25));
        query.addBindValue(obstracles.at(i).at(26));
        query.addBindValue(idAirfield);
        if (!query.exec()) {
            qDebug() << query.lastError().text() << " " << query.lastQuery();
        }
    }
    if (!QSqlDatabase::database().commit()) {
        QSqlDatabase::database().rollback();
        qDebug() << "Rollback transaction";
    }
}
