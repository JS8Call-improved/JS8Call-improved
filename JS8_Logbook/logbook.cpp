/**
 * @file logbook.cpp
 * @brief Implementation of LogBook class
 */
#include "logbook.h"
#include <QDebug>
#include <QFontMetrics>
#include <QStandardPaths>
#include <QDir>

namespace
{
  auto logFileName = "js8call_log.adi";
  auto countryFileName = "cty.dat";
}

/**
 * @brief Initialize the logbook by loading country data and existing log entries.
 */

void LogBook::init()
{
  QDir dataPath {QStandardPaths::writableLocation (QStandardPaths::AppLocalDataLocation)};
  QString countryDataFilename;
  if (dataPath.exists (countryFileName))
    {
      // User override
      countryDataFilename = dataPath.absoluteFilePath (countryFileName);
    }
  else
    {
      countryDataFilename = QString {":/"} + countryFileName;
    }

  _countries.init(countryDataFilename);
  _countries.load();

  _worked.init(_countries.getCountryNames());

  _log.init(dataPath.absoluteFilePath (logFileName));
  _log.load();

  _setAlreadyWorkedFromLog();
}

/**
 * @brief Populate the CountriesWorked instance based on existing log entries.
 */
void LogBook::_setAlreadyWorkedFromLog()
{
  QList<QString> calls = _log.getCallList();
  QString c;
  foreach(c,calls)
    {
      QString countryName = _countries.find(c);
      if (countryName.length() > 0)
        {
          _worked.setAsWorked(countryName);
        }
    }
}

/**
 * @brief Check if a call has been worked before on a specific band.
 * @param call The callsign to check.
 * @param band The band to check.
 * @return True if the call has been worked before on the specified band, false otherwise.
 */
bool LogBook::hasWorkedBefore(const QString &call, const QString &band){
    return _log.match(call, band);
}

/**
 * @brief Match a callsign to its country and check if it has been worked before.
 * @param call The callsign to match.
 * @param countryName Output parameter to hold the matched country name.
 * @param callWorkedBefore Output parameter indicating if the call has been worked before.
 * @param countryWorkedBefore Output parameter indicating if the country has been worked before.
 */
void LogBook::match(/*in*/const QString call,
                    /*out*/ QString &countryName,
                    bool &callWorkedBefore,
                    bool &countryWorkedBefore) const
{
    if(call.isEmpty()){
        return;
    }

    QString currentBand = "";  // match any band
    callWorkedBefore = _log.match(call, currentBand);
    countryName = _countries.find(call);

    if (countryName.length() > 0){  //  country was found
        countryWorkedBefore = _worked.getHasWorked(countryName);
    } else {
        countryName = "where?"; //error: prefix not found
        countryWorkedBefore = false;
    }
}

/**
 * @brief Find details associated with a callsign in the logbook.
 * @param call The callsign to search for.
 * @param grid Output parameter to hold the grid locator.
 * @param date Output parameter to hold the date of the QSO.
 * @param name Output parameter to hold the name of the operator.
 * @param comment Output parameter to hold any comments associated with the QSO.
 * @return True if details were found, false otherwise.
 */
bool LogBook::findCallDetails(
                    /*in*/
                    const QString call,
                    /*out*/
                    QString &grid,
                    QString &date,
                    QString &name,
                    QString &comment) const
{
    if(call.isEmpty()){
        return false;
    }

    auto qsos = _log.find(call);
    if(qsos.isEmpty()){
        return false;
    }

    foreach(auto qso, qsos){
        if(grid.isEmpty() && !qso.grid.isEmpty()) grid = qso.grid;
        if(date.isEmpty() && !qso.date.isEmpty()) date = qso.date;
        if(name.isEmpty() && !qso.name.isEmpty()) name = qso.name;
        if(comment.isEmpty() && !qso.comment.isEmpty()) comment = qso.comment;
    }

    return true;
}

/**
 * @brief Add a new QSO to the logbook and mark the country as worked.
 * @param call The callsign of the contacted station.
 * @param band The band on which the QSO was made.
 * @param mode The mode of the QSO.
 * @param submode The submode of the QSO.
 * @param grid The grid locator of the contacted station.
 * @param date The date of the QSO.
 * @param name The name of the operator.
 * @param comment Any comments associated with the QSO.
 */
void LogBook::addAsWorked(const QString call, const QString band, const QString mode, const QString submode, const QString grid, const QString date, const QString name, const QString comment)
{
  _log.add(call,band,mode,submode,grid,date,name,comment);
  QString countryName = _countries.find(call);
  if (countryName.length() > 0)
    _worked.setAsWorked(countryName);
}
