/*
 * From an ADIF file and cty.dat, get a call's DXCC entity and its worked before status
 * VK3ACF July 2013
 */

#ifndef LOGBOOK_H
#define LOGBOOK_H


#include <QString>
#include <QFont>

#include "countrydat.h"
#include "countriesworked.h"
#include "adif.h"
#include "n3fjp.h"

class QDir;

class LogBook
{
public:
    void init();
    bool hasWorkedBefore(const QString &call, const QString &band);
    void match(/*in*/ const QString call,
              /*out*/ QString &countryName,
                      bool &callWorkedBefore,
                      bool &countryWorkedBefore) const;
    bool findCallDetails(/*in*/
                        const QString call,
                        /*out*/
                        QString &grid,
                        QString &date,
                        QString &name,
                        QString &comment) const;
    void addAsWorked(const QString call, const QString band, const QString mode, const QString submode, const QString grid, const QString date, const QString name, const QString comment);

private:
   CountryDat _countries;
   CountriesWorked _worked;
   ADIF _log;

   void _setAlreadyWorkedFromLog();

};

#endif // LOGBOOK_H

