/**
 * @file adif.cpp
 * @brief Implementation of the ADIF class for handling ADIF log files.
 */
#include "adif.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(adif_js8)


const QStringList ADIF_FIELDS = {
    // ADIF 3.1.0 - pulled from http://www.adif.org/310/adx310.xsd on 2019-06-04
    "APP",
    "ADDRESS",
    "ADDRESS_INTL",
    "AGE",
    "A_INDEX",
    "ANT_AZ",
    "ANT_EL",
    "ANT_PATH",
    "ARRL_SECT",
    "AWARD_SUBMITTED",
    "AWARD_GRANTED",
    "BAND",
    "BAND_RX",
    "CALL",
    "CHECK",
    "CLASS",
    "CLUBLOG_QSO_UPLOAD_DATE",
    "CLUBLOG_QSO_UPLOAD_STATUS",
    "CNTY",
    "COMMENT",
    "COMMENT_INTL",
    "CONT",
    "CONTACTED_OP",
    "CONTEST_ID",
    "COUNTRY",
    "COUNTRY_INTL",
    "CQZ",
    "CREDIT_SUBMITTED",
    "CREDIT_GRANTED",
    "DARC_DOK",
    "DISTANCE",
    "DXCC",
    "EMAIL",
    "EQ_CALL",
    "EQSL_QSLRDATE",
    "EQSL_QSLSDATE",
    "EQSL_QSL_RCVD",
    "EQSL_QSL_SENT",
    "FISTS",
    "FISTS_CC",
    "FORCE_INIT",
    "FREQ",
    "FREQ_RX",
    "GRIDSQUARE",
    "GUEST_OP",
    "HRDLOG_QSO_UPLOAD_DATE",
    "HRDLOG_QSO_UPLOAD_STATUS",
    "IOTA",
    "IOTA_ISLAND_ID",
    "ITUZ",
    "K_INDEX",
    "LAT",
    "LON",
    "LOTW_QSLRDATE",
    "LOTW_QSLSDATE",
    "LOTW_QSL_RCVD",
    "LOTW_QSL_SENT",
    "MAX_BURSTS",
    "MODE",
    "MS_SHOWER",
    "MY_ANTENNA",
    "MY_ANTENNA_INTL",
    "MY_CITY",
    "MY_CITY_INTL",
    "MY_CNTY",
    "MY_COUNTRY",
    "MY_COUNTRY_INTL",
    "MY_CQ_ZONE",
    "MY_DXCC",
    "MY_FISTS",
    "MY_GRIDSQUARE",
    "MY_IOTA",
    "MY_IOTA_ISLAND_ID",
    "MY_ITU_ZONE",
    "MY_LAT",
    "MY_LON",
    "MY_NAME",
    "MY_NAME_INTL",
    "MY_POSTAL_CODE",
    "MY_POSTAL_CODE_INTL",
    "MY_RIG",
    "MY_RIG_INTL",
    "MY_SIG",
    "MY_SIG_INTL",
    "MY_SIG_INFO",
    "MY_SIG_INFO_INTL",
    "MY_SOTA_REF",
    "MY_STATE",
    "MY_STREET",
    "MY_STREET_INTL",
    "MY_USACA_COUNTIES",
    "MY_VUCC_GRIDS",
    "NAME",
    "NAME_INTL",
    "NOTES",
    "NOTES_INTL",
    "NR_BURSTS",
    "NR_PINGS",
    "OPERATOR",
    "OWNER_CALLSIGN",
    "PFX",
    "PRECEDENCE",
    "PROP_MODE",
    "PUBLIC_KEY",
    "QRZCOM_QSO_UPLOAD_DATE",
    "QRZCOM_QSO_UPLOAD_STATUS",
    "QSLMSG",
    "QSLMSG_INTL",
    "QSLRDATE",
    "QSLSDATE",
    "QSL_RCVD",
    "QSL_RCVD_VIA",
    "QSL_SENT",
    "QSL_SENT_VIA",
    "QSL_VIA",
    "QSO_COMPLETE",
    "QSO_DATE",
    "QSO_DATE_OFF",
    "QSO_RANDOM",
    "QTH",
    "QTH_INTL",
    "REGION",
    "RIG",
    "RIG_INTL",
    "RST_RCVD",
    "RST_SENT",
    "RX_PWR",
    "SAT_MODE",
    "SAT_NAME",
    "SFI",
    "SIG",
    "SIG_INTL",
    "SIG_INFO",
    "SIG_INFO_INTL",
    "SILENT_KEY",
    "SKCC",
    "SOTA_REF",
    "SRX",
    "SRX_STRING",
    "STATE",
    "STATION_CALLSIGN",
    "STX",
    "STX_STRING",
    "SUBMODE",
    "SWL",
    "TEN_TEN",
    "TIME_OFF",
    "TIME_ON",
    "TX_PWR",
    "UKSMG",
    "USACA_COUNTIES",
    "VUCC_GRIDS",
    "WEB",
};

/*
<CALL:4>W1XT<BAND:3>20m<FREQ:6>14.076<GRIDSQUARE:4>DM33<MODE:4>JT65<RST_RCVD:3>-21<RST_SENT:3>-14<QSO_DATE:8>20110422<TIME_ON:6>041712<TIME_OFF:6>042435<TX_PWR:1>4<COMMENT:34>1st JT65A QSO.   Him: mag loop 20W<STATION_CALLSIGN:6>VK3ACF<MY_GRIDSQUARE:6>qf22lb<eor>
<CALL:6>IK1SOW<BAND:3>20m<FREQ:6>14.076<GRIDSQUARE:4>JN35<MODE:4>JT65<RST_RCVD:3>-19<RST_SENT:3>-11<QSO_DATE:8>20110422<TIME_ON:6>052501<TIME_OFF:6>053359<TX_PWR:1>3<STATION_CALLSIGN:6>VK3ACF<MY_GRIDSQUARE:6>qf22lb<eor>
<CALL:6:S>W4ABC> ...
*/

/**
 * @brief Initialize the ADIF instance with the specified filename.
 * @param filename The path to the ADIF log file.
 */
void ADIF::init(QString const& filename)
{
    _filename = filename;
    _data.clear();
}

/**
 * @brief Extract the value of a specified field from an ADIF record.
 * @param record The ADIF record as a string.
 * @param fieldName The name of the field to extract.
 * @return The extracted field value, or an empty string if not found.
 */
QString ADIF::extractField(QString const& record, QString const& fieldName) const
{
    qsizetype fieldNameIndex = record.indexOf ('<' + fieldName + ':', 0, Qt::CaseInsensitive);
    if (fieldNameIndex >=0)
    {
        qsizetype closingBracketIndex = record.indexOf('>',fieldNameIndex);
        qsizetype fieldLengthIndex    = record.indexOf(':',fieldNameIndex);  // find the size delimiter
        qsizetype dataTypeIndex       = -1;
        if (fieldLengthIndex >= 0)
        {
          dataTypeIndex = record.indexOf(':',fieldLengthIndex+1);  // check for a second : indicating there is a data type
          if (dataTypeIndex > closingBracketIndex)
            dataTypeIndex = -1; // second : was found but it was beyond the closing >
        }

        if ((closingBracketIndex > fieldNameIndex) && (fieldLengthIndex > fieldNameIndex) && (fieldLengthIndex< closingBracketIndex))
        {
            qsizetype fieldLengthCharCount = closingBracketIndex - fieldLengthIndex -1;
            if (dataTypeIndex >= 0)
              fieldLengthCharCount -= 2; // data type indicator is always a colon followed by a single character
            QString fieldLengthString = record.mid(fieldLengthIndex+1,fieldLengthCharCount);
            int fieldLength = fieldLengthString.toInt();
            if (fieldLength > 0)
            {
              QString field = record.mid(closingBracketIndex+1,fieldLength);
              return field;
            }
       }
    }
    return "";
}

/**
 * @brief Load ADIF records from the specified file into the internal data structure.
 */
void ADIF::load()
{
    _data.clear();
    QFile inputFile(_filename);
    if (inputFile.open(QIODevice::ReadOnly))
    {
      QTextStream in(&inputFile);
      QString buffer;
      bool pre_read {false};
      qsizetype end_position {-1};

      // skip optional header record
      do
        {
          buffer += in.readLine () + '\n';
          if (buffer.startsWith (QChar {'<'})) // denotes no header
            {
              pre_read = true;
            }
          else
            {
              end_position = buffer.indexOf ("<EOH>", 0, Qt::CaseInsensitive);
            }
        }
      while (!in.atEnd () && !pre_read && end_position < 0);
      if (!pre_read)            // found header
        {
          buffer.remove (0, end_position + 5);
        }
      while (buffer.size () || !in.atEnd ())
        {
          do
            {
              end_position = buffer.indexOf ("<EOR>", 0, Qt::CaseInsensitive);
              if (!in.atEnd () && end_position < 0)
                {
                  buffer += in.readLine () + '\n';
                }
            }
          while (!in.atEnd () && end_position < 0);
          qsizetype record_length {end_position >= 0 ? end_position + 5 : -1};
          auto record = buffer.left (record_length).trimmed ();
          auto next_record = buffer.indexOf (QChar {'<'}, record_length);
          buffer.remove (0, next_record >=0 ? next_record : buffer.size ());
          record = record.mid (record.indexOf (QChar {'<'}));
          add (extractField (record, "CALL")
               , extractField (record, "BAND")
               , extractField (record, "MODE")
               , extractField (record, "SUBMODE")
               , extractField (record, "GRIDSQUARE")
               , extractField (record, "QSO_DATE")
               , extractField (record, "NAME")
               , extractField (record, "COMMENT")
               );
        }
        inputFile.close ();
    }
}

/**
 * @brief Add a new QSO to the internal data structure.
 * @param call The callsign of the contacted station.
 * @param band The band on which the QSO was made.
 * @param mode The mode of the QSO.
 * @param submode The submode of the QSO.
 * @param grid The grid locator of the contacted station.
 * @param date The date of the QSO.
 * @param name The name of the operator.
 * @param comment Any comments associated with the QSO.
 */
void ADIF::add(QString const& call, QString const& band, QString const& mode, QString const& submode, QString const &grid, QString const& date, QString const& name, QString const& comment)
{
    QSO q;
    q.call = call;
    q.band = band;
    q.mode = mode;
    q.submode = submode;
    q.grid = grid;
    q.date = date;
    q.name = name;
    q.comment = comment;

    if (q.call.size ())
      {
        _data.insert(q.call,q);
        // qCDebug(adif_js8) << "Added as worked:" << call << band << mode << date;
      }
}

/**
 * @brief Check if a callsign and band combination exists in the internal data structure.
 * @param call The callsign to search for.
 * @param band The band to search for.
 * @return True if a matching QSO is found, false otherwise.
 */
bool ADIF::match(QString const& call, QString const& band) const
{
    QList<QSO> qsos = _data.values(call);
    if (qsos.size()>0)
    {
        QSO q;
        foreach(q,qsos)
        {
            if (     (band.compare(q.band,Qt::CaseInsensitive) == 0)
                  || (band=="")
                  || (q.band==""))
            {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Find QSOs associated with a given callsign.
 * @param call The callsign to search for.
 * @return A list of QSOs associated with the callsign.
 */
QList<ADIF::QSO> ADIF::find(QString const& call) const
{
    return _data.values(call);
}

/**
 * @brief Get a list of all callsigns in the internal data structure.
 * @return A list of callsigns.
 */
QList<QString> ADIF::getCallList() const
{
    QList<QString> p;
    QMultiHash<QString,QSO>::const_iterator i = _data.constBegin();
     while (i != _data.constEnd())
     {
         p << i.key();
         ++i;
     }
    return p;
}

/**
 * @brief Get the count of QSOs in the internal data structure.
 * @return The number of QSOs.
 */
qsizetype ADIF::getCount() const
{
    return _data.size();
}

/**
 * @brief Convert QSO details into an ADIF record format.
 * @param hisCall The callsign of the contacted station.
 * @param hisGrid The grid locator of the contacted station.
 * @param mode The mode of the QSO.
 * @param submode The submode of the QSO.
 * @param rptSent The report sent.
 * @param rptRcvd The report received.
 * @param dateTimeOn The date and time when the QSO started.
 * @param dateTimeOff The date and time when the QSO ended.
 * @param band The band on which the QSO was made.
 * @param comments Any comments associated with the QSO.
 * @param name The name of the operator.
 * @param strDialFreq The dial frequency as a string.
 * @param m_myCall The station's own callsign.
 * @param m_myGrid The station's own grid locator.
 * @param operator_call The operator's callsign.
 * @param additionalFields A map of additional ADIF fields to include.
 * @return The ADIF record as a QByteArray.
 */
QByteArray ADIF::QSOToADIF(QString const& hisCall, QString const& hisGrid, QString const& mode, QString const& submode
                           , QString const& rptSent, QString const& rptRcvd, QDateTime const& dateTimeOn
                           , QDateTime const& dateTimeOff, QString const& band, QString const& comments
                           , QString const& name, QString const& strDialFreq, QString const& m_myCall
                           , QString const& m_myGrid, QString const& operator_call, QMap<QString, QVariant> const &additionalFields)
{
  QString t;
  t = "<call:" + QString::number(hisCall.length()) + ">" + hisCall;
  t += " <gridsquare:" + QString::number(hisGrid.length()) + ">" + hisGrid;
  t += " <mode:" + QString::number(mode.length()) + ">" + mode;
  if(!submode.isEmpty()){
    t += " <submode:" + QString::number(submode.length()) + ">" + submode;
  }
  t += " <rst_sent:" + QString::number(rptSent.length()) + ">" + rptSent;
  t += " <rst_rcvd:" + QString::number(rptRcvd.length()) + ">" + rptRcvd;
  t += " <qso_date:8>" + dateTimeOn.date().toString("yyyyMMdd");
  t += " <time_on:6>" + dateTimeOn.time().toString("hhmmss");
  t += " <qso_date_off:8>" + dateTimeOff.date().toString("yyyyMMdd");
  t += " <time_off:6>" + dateTimeOff.time().toString("hhmmss");
  t += " <band:" + QString::number(band.length()) + ">" + band;
  t += " <freq:" + QString::number(strDialFreq.length()) + ">" + strDialFreq;
  t += " <station_callsign:" + QString::number(m_myCall.length()) + ">" +
      m_myCall;
  t += " <my_gridsquare:" + QString::number(m_myGrid.length()) + ">" +
      m_myGrid;
  if (comments != "")
    t += " <comment:" + QString::number(comments.length()) +
        ">" + comments;
  if (name != "")
    t += " <name:" + QString::number(name.length()) +
        ">" + name;
  if (operator_call!="")
      t+=" <operator:" + QString::number(operator_call.length()) +
              ">" + operator_call;

  foreach(auto key, additionalFields.keys()){
      auto k = key.toUpper();
      auto value = additionalFields[k].toString();

      if(ADIF_FIELDS.contains(k)){
        t += QString(" <%1:%2>%3").arg(k).arg(value.length()).arg(value);
      } else {
        t += QString(" <APP_JS8CALL_%1:%2>%3").arg(k).arg(value.length()).arg(value);
      }
  }

  return t.toLatin1 ();
}

/**
 * @brief Open the ADIF file and append the QSO details.
 * @param ADIF_record The ADIF record to append.
 * @return True on success, false otherwise.
 */
bool ADIF::addQSOToFile(QByteArray const& ADIF_record)
{
    QFile f2(_filename);
    if (!f2.open(QIODevice::Text | QIODevice::Append))
        return false;
    else
    {
        QTextStream out(&f2);
        if (f2.size()==0)
            out << "JS8Call ADIF Export<eoh>" << Qt::endl;  // new file

        out << ADIF_record << " <eor>" << Qt::endl;
        out.flush();
        flushFileBuffer(f2);
        f2.close();
    }
    return true;
}

Q_LOGGING_CATEGORY(adif_js8, "adif.js8", QtWarningMsg)
