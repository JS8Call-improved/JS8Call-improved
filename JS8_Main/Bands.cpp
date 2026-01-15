/**
 * @file Bands.cpp
 * @brief Implementation of Bands model
 */
#include "Bands.h"

#include <QString>
#include <QVariant>
#include <algorithm>

namespace
{
// Table of ADIF band definitions as defined in the ADIF
// specification as at ADIF v3.0.6
struct ADIFBand
{
    char const* const name_;
    Radio::Frequency lower_bound_;
    Radio::Frequency upper_bound_;
} constexpr ADIF_bands[] = {
    { "2190m",  136000u,          137000u          },
    { "630m",   472000u,          479000u          },
    { "560m",   501000u,          504000u          },
    { "160m",   1'800'000u,       2'000'000u       },
    { "80m",    3'500'000u,       4'000'000u       },
    { "60m",    5'060'000u,       5'450'000u       },
    { "40m",    7'000'000u,       7'300'000u       },
    { "30m",    10'000'000u,      10'150'000u      },
    { "20m",    14'000'000u,      14'350'000u      },
    { "17m",    18'068'000u,      18'168'000u      },
    { "15m",    21'000'000u,      21'450'000u      },
    { "12m",    24'890'000u,      24'990'000u      },
    { "10m",    28'000'000u,      29'700'000u      },
    { "6m",     50'000'000u,      54'000'000u      },
    { "4m",     70'000'000u,      71'000'000u      },
    { "2m",     144'000'000u,     148'000'000u     },
    { "1.25m",  222'000'000u,     225'000'000u     },
    { "70cm",   420'000'000u,     450'000'000u     },
    { "33cm",   902'000'000u,     928'000'000u     },
    { "23cm",   1'240'000'000u,   1'300'000'000u   },
    { "13cm",   2'300'000'000u,   2'450'000'000u   },
    { "9cm",    3'300'000'000u,   3'500'000'000u   },
    { "6cm",    5'650'000'000u,   5'925'000'000u   },
    { "3cm",    10'000'000'000u,  10'500'000'000u  },
    { "1.25cm", 24'000'000'000u,  24'250'000'000u  },
    { "6mm",    47'000'000'000u,  47'200'000'000u  },
    { "4mm",    75'500'000'000u,  81'000'000'000u  },
    { "2.5mm",  119'980'000'000u, 120'020'000'000u },
    { "2mm",    142'000'000'000u, 149'000'000'000u },
    { "1mm",    241'000'000'000u, 250'000'000'000u },
};

/**
 * @brief Out Of Band name
 * 
 */
QString const oob_name { QObject::tr("OOB") };

/**
 * @brief Get number of rows in ADIF band table
 * 
 * @return int 
 */
int constexpr table_rows()
{
    return sizeof(ADIF_bands) / sizeof(ADIF_bands[0]);
}
} // namespace

Bands::Bands(QObject* parent) : QAbstractTableModel { parent }
{
}

/**
 * @brief Find the band that contains the given frequency
 * 
 * @param f 
 * @return QString 
 */
QString Bands::find(Frequency f) const
{
    QString result;
    auto const& end_iter = ADIF_bands + table_rows();
    auto const& row_iter = std::find_if(ADIF_bands, end_iter, [f](ADIFBand const& band) {
        return band.lower_bound_ <= f && f <= band.upper_bound_;
    });
    if (row_iter != end_iter) {
        result = row_iter->name_;
    }
    return result;
}

/**
 * @brief Find the row index of the given band name
 * 
 * @param band 
 * @return int 
 */
int Bands::find(QString const& band) const
{
    int result { -1 };
    for (auto i = 0u; i < table_rows(); ++i) {
        if (band == ADIF_bands[i].name_) {
            result = i;
        }
    }
    return result;
}

/**
 * @brief Find the frequency bounds for the given band name
 * 
 * @param band 
 * @param pFreqLower 
 * @param pFreqHigher 
 * @return true if band found, false otherwise
 */
bool Bands::findFreq(QString const& band,
                     Radio::Frequency* pFreqLower,
                     Radio::Frequency* pFreqHigher) const
{
    int row = find(band);
    if (row == -1) {
        return false;
    }

    if (pFreqLower)
        *pFreqLower = ADIF_bands[row].lower_bound_;
    if (pFreqHigher)
        *pFreqHigher = ADIF_bands[row].upper_bound_;

    return true;
}

/**
 * @brief Get Out Of Band name
 * 
 * @return QString const& 
 */
QString const& Bands::oob()
{
    return oob_name;
}

/**
 * @brief Get number of rows in the model
 * 
 * @param parent 
 * @return int 
 */
int Bands::rowCount(QModelIndex const& parent) const
{
    return parent.isValid() ? 0 : table_rows();
}

/**
 * @brief Get number of columns in the model
 * 
 * @param parent 
 * @return int 
 */
int Bands::columnCount(QModelIndex const& parent) const
{
    return parent.isValid() ? 0 : 3;
}

/**
 * @brief Get item flags for the given index
 * 
 * @param index 
 * @return Qt::ItemFlags 
 */
Qt::ItemFlags Bands::flags(QModelIndex const& index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsDropEnabled;
}

/**
 * @brief Get data for the given index and role
 * 
 * @param index 
 * @param role 
 * @return QVariant 
 */
QVariant Bands::data(QModelIndex const& index, int role) const
{
    QVariant item;

    if (!index.isValid()) {
        // Hijack root for OOB string.
        if (Qt::DisplayRole == role) {
            item = oob_name;
        }
    } else {
        auto row = index.row();
        auto column = index.column();

        if (row < table_rows()) {
            switch (role) {
            case Qt::ToolTipRole:
            case Qt::AccessibleDescriptionRole:
                switch (column) {
                case 0: item = tr("Band name"); break;
                case 1: item = tr("Lower frequency limit"); break;
                case 2: item = tr("Upper frequency limit"); break;
                }
                break;

            case SortRole:
            case Qt::DisplayRole:
            case Qt::EditRole:
                switch (column) {
                case 0:
                    if (SortRole == role) {
                        // band name sorts by lower bound
                        item = ADIF_bands[row].lower_bound_;
                    } else {
                        item = ADIF_bands[row].name_;
                    }
                    break;

                case 1: item = ADIF_bands[row].lower_bound_; break;
                case 2: item = ADIF_bands[row].upper_bound_; break;
                }
                break;

            case Qt::AccessibleTextRole:
                switch (column) {
                case 0: item = ADIF_bands[row].name_; break;
                case 1: item = ADIF_bands[row].lower_bound_; break;
                case 2: item = ADIF_bands[row].upper_bound_; break;
                }
                break;

            case Qt::TextAlignmentRole:
                switch (column) {
                case 0: item = Qt::AlignCenter; break;

                case 1:
                case 2:
                    item = static_cast<Qt::Alignment::Int>(Qt::AlignRight | Qt::AlignVCenter);
                    break;
                }
                break;
            }
        }
    }
    return item;
}

/**
 * @brief Get header data for the given section, orientation, and role
 * 
 * @param section 
 * @param orientation 
 * @param role 
 * @return QVariant 
 */
QVariant Bands::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant result;

    if (Qt::DisplayRole == role && Qt::Horizontal == orientation) {
        switch (section) {
        case 0: result = tr("Band"); break;
        case 1: result = tr("Lower Limit"); break;
        case 2: result = tr("Upper Limit"); break;
        }
    } else {
        result = QAbstractTableModel::headerData(section, orientation, role);
    }

    return result;
}

/**
 * @brief Dereference the iterator to get the band name
 * 
 * @return QString 
 */
QString Bands::const_iterator::operator*()
{
    return ADIF_bands[row_].name_;
}

/**
 * @brief Compare two iterators for inequality
 * 
 * @param rhs 
 * @return true if not equal, false otherwise
 */
bool Bands::const_iterator::operator!=(const_iterator const& rhs) const
{
    return row_ != rhs.row_;
}

/**
 * @brief Increment the iterator
 * 
 * @return reference to incremented iterator
 */
auto Bands::const_iterator::operator++() -> const_iterator&
{
    ++row_;
    return *this;
}

/**
 * @brief Get begin iterator
 * 
 * @return Bands::const_iterator 
 */
auto Bands::begin() const -> Bands::const_iterator
{
    return const_iterator(0);
}

/**
 * @brief Get end iterator
 * 
 * @return Bands::const_iterator 
 */
auto Bands::end() const -> Bands::const_iterator
{
    return const_iterator(table_rows());
}
