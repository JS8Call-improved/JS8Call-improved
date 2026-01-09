/**
 * @file countriesworked.cpp
 * @brief Implementation of the CountriesWorked class for tracking worked countries.
 */
#include "countriesworked.h"

/**
 * @brief Initialize the CountriesWorked instance with the specified country names.
 * @param countryNames A list of country names to track.
 */
void CountriesWorked::init(const QStringList countryNames)
{
    _data.clear();
    foreach(QString name,countryNames)
      _data.insert(name,false);
}

/**
 * @brief Mark a country as worked.
 * @param countryName The name of the country to mark as worked.
 */
void CountriesWorked::setAsWorked(const QString countryName)
{
    if (_data.contains(countryName))
      _data.insert(countryName,true);
}

/**
 * @brief Check if a country has been worked.
 * @param countryName The name of the country to check.
 * @return True if the country has been worked, false otherwise.
 */
bool CountriesWorked::getHasWorked(const QString countryName) const
{
    if (_data.contains(countryName))
      return _data.value(countryName);

    return false;
}

/**
 * @brief Get the count of countries that have been worked.
 * @return The number of worked countries.
 */
qsizetype CountriesWorked::getWorkedCount() const
{
    qsizetype count = 0;
	foreach (bool value,_data)
		if (value)
			count += 1;
    return count;
}

/**
 * @brief Get the total number of countries being tracked.
 * @return The total number of countries.
 */
qsizetype CountriesWorked::getSize() const
{
    return _data.count();
}



