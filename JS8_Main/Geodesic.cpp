#include "Geodesic.h"
#include "JS8_Include/Maidenhead.h"
#include <QCache>
#include <QMutex>
#include <QMutexLocker>
#include <concepts>

/******************************************************************************/
// Constants
/******************************************************************************/

namespace {
// Epsilon values for Lat / Long comparisons; if we find after conversion
// to coordinates that a pair of grids are either identical or antipodes,
// within these limits, we'll return early rather than computing results.

constexpr auto LL_EPSILON_IDENTICAL = 0.02f;
constexpr auto LL_EPSILON_ANTIPODES = 1.e-6f;

// Table of compass directions, with arrows; available as an accessor on
// the Azimuth class. These are translatable, but translation is not our
// concern; we simply guarantee to the presentation layer that these are
// what we provide, deferring translation to somewhere above us.

constexpr QStringView COMPASS[] = {u"\u2191N",  u"\u2197NE", u"\u2192E",
                                   u"\u2198SE", u"\u2193S",  u"\u2199SW",
                                   u"\u2190W",  u"\u2196NW"};
} // namespace

/******************************************************************************/
// Input Normalization
/******************************************************************************/

namespace {
// Structure used to perform lookups; represents normalized, i.e.,
// validated, trimmed fore and aft, converted to upper case, grid
// identifiers, and an indication if either are only sufficiently
// long to contain square data, rather than subsquare or extended.

struct Data {
    QString origin;
    QString remote;
    bool square;
};

// Given a pair of strings that have passed validity checking, create
// and return normalized data.

auto normalize(QStringView const origin, QStringView const remote) {
    auto const normalizedOrigin = origin.trimmed().toString().toUpper();
    auto const normalizedRemote = remote.trimmed().toString().toUpper();

    return Data{normalizedOrigin, normalizedRemote,
                normalizedOrigin.length() < 6 || normalizedRemote.length() < 6};
}
} // namespace

/******************************************************************************/
// Grid Square to Coordinates
/******************************************************************************/

namespace {
// Grid to coordinate transformation, with results exactly matching those
// of the Fortran subroutine grid2deg() within the domain of grid2deg(),
// which is only up to subsquare. Input is a 4, 6, or 8 character grid
// square, validated and normalized by the functions above.

template <qsizetype Offset> auto gridData(QStringView const grid) {
    static_assert(Offset == 0 || Offset == 1);
    return std::array{
        grid[Offset].unicode() - u'A', grid[Offset + 2].unicode() - u'0',
        (grid.size() > 4 ? grid[Offset + 4].unicode() : u'M') - u'A',
        (grid.size() > 6 ? grid[Offset + 6].unicode() : u'4') - u'0'};
}

inline auto gridLat(QStringView const grid) {
    // 1: A-R,  10° each, field, one of 18 zones of latitude
    // 3: 0-9,   1° each, 100 squares within field
    // 5: A-X, 2.5' each, 576 subsquares within square
    // 7: 0-9,  15" each, 100 extended squares within subsquare

    auto const data = gridData<1>(grid);

    return (-90 + 10 * data[0]) + data[1] +
           ((2.5f * (data[2] + 0.5f)) / 60.0f) +
           ((15 * (data[3] + 0.5f)) / 3600.0f);
}

inline auto gridLon(QStringView const grid) {
    // 0: A-R, 20° each, field, one of 18 zones of longitude
    // 2: 0-9,  2° each, 100 squares within field
    // 4: A-X,  5' each, 576 subsquares within square
    // 6: 0-9, 30" each, 100 extended squares within subsquare

    auto const data = gridData<0>(grid);

    return (180 - 20 * data[0]) - (2 * data[1]) -
           ((5 * (data[2] + 0.5f)) / 60.0f) -
           ((30 * (data[3] + 0.5f)) / 3600.0f);
}

class Coords {
  private:
    // Data members

    float m_lat;
    float m_lon;

  public:
    // Inline Accessors

    auto lat() const { return m_lat; }
    auto lon() const { return m_lon; }

    // Constructor

    Coords(QStringView const grid)
        : m_lat(gridLat(grid)), m_lon(gridLon(grid)) {}

    // Determine if these coordinates are identical to those provided,
    // within the defined epsilon limit.

    bool isIdenticalTo(Coords const other) const {
        auto const latValue = std::abs(lat() - other.lat());
        auto const lonValue = std::abs(lon() - other.lon());

        return ((latValue < LL_EPSILON_IDENTICAL) &&
                (lonValue < LL_EPSILON_IDENTICAL));
    }

    // Determine if these coordinates are antipodes of those provided,
    // within the defined epsilon limit.

    bool isAntipodesOf(Coords const other) const {
        // We subtract the longitudes and add 720 degrees to ensure
        // a positive result; modulo 360 of that results in a value
        // in the range [-180, 180].

        auto const range = std::fmod(lon() - other.lon() + 720.0f, 360.0f);
        auto const latValue = std::abs(lat() + other.lat());
        auto const lonValue = std::abs(range - 180.0f);

        return ((latValue < LL_EPSILON_ANTIPODES) &&
                (lonValue < LL_EPSILON_ANTIPODES));
    }
};
} // namespace

/******************************************************************************/
// Coordinates to Azimuth / Distance
/******************************************************************************/

namespace {
// Collapsed and simplified versions of two of JHT's original Fortran
// subroutines, azdist() and geodist(). Given normalized data, return
// the azimuth in degrees and the distance in kilometers.
//
// While in a perfect world, we could use the Haversine distance, a
// perfect world is a sphere, and ours is an ellipsoid, flattened at
// the poles. Thus, there is...math.
//
// Boy howdy, there is math.
//
// Note that as with the original routines, West longitude is positive.

auto azdist(Data const &data) {
    // If they've given us the same grids, reward them appropriately.

    if (data.origin == data.remote)
        return std::make_pair(0.0f, 0.0f);

    // Convert the grids to coordinates.

    auto const origin = Coords{data.origin};
    auto const remote = Coords{data.remote};

    // Grids that looked different prior to conversion to coordinates
    // can nevertheless be practically on top of one another; we can't
    // go there, because we're already there.
    //
    // Grids that are antipodes of one another aren't worth calculating;
    // you can't get farther away without leaving the planet, moving in
    // any direction will take you there, and it's the same distance no
    // matter what direction you go.

    if (origin.isIdenticalTo(remote))
        return std::make_pair(0.0f, 0.0f);
    if (origin.isAntipodesOf(remote))
        return std::make_pair(0.0f, 204000.0f);

    // Sanity checks complete; let's do some math. JHT took this algorithm
    // from:
    //
    //   Thomas, P.D., 1970,
    //   Spheroidal Geodesics, Reference Systems, & Local Geometry,
    //   U.S. Naval Oceanographic Office SP-138, 165 pp.
    //
    //    "A discussion of the geodesic on the oblate spheroid (reference
    //     ellipsoid) is given with formulae of geodetic accuracy (second
    //     order in the flattening, distance and azimuths) for the non-
    //     iterative direct and inverse solutions over the hemispheroid,
    //     requiring no root extraction and no tabular data except 3-place
    //     tables of the natural trigonometric functions."
    //
    // This is a C++ translation of the original Fortran-63 algorithm, pages
    // 162 and 163 of the publication, description on page 161.
    //
    // Note that the original routine uses single-precision real values, so
    // we've done the same here. The degrees to radians and Tau constants
    // used by JHT's version are slightly different than those used by the
    // Thomas algorithm; we've used the JHT versions.
    //
    // Constants and derived constants:
    //
    //   AL:  Semi-major axis of the Earth, Clarke 1866 ellipsoid, in km.
    //   BL:  Semi-minor axis of the Earth, Clarke 1866 ellipsoid, in km.
    //   D2R: Degrees to radians conversion factor.
    //   TAU: Tau constant, the ratio of the circumference to the radius
    //        of a circle, i.e, 2Pi;

    constexpr auto AL = 6378206.4f;
    constexpr auto BL = 6356583.8f;
    constexpr auto D2R = 0.01745329251994f;
    constexpr auto TAU = 6.28318530718f;
    constexpr auto BOA = BL / AL;
    constexpr auto F = 1.0f - BOA;
    constexpr auto FF64 = F * F / 64.0f;

    // Convert degrees of latitude and longitude to radians and compute the
    // delta longitude in radians.

    auto const P1R = origin.lat() * D2R;
    auto const P2R = remote.lat() * D2R;
    auto const L1R = origin.lon() * D2R;
    auto const L2R = remote.lon() * D2R;
    auto const DLR = L2R - L1R;

    // And away we go; see page 162 of the publication.

    auto const T1R = std::atan(BOA * std::tan(P1R));
    auto const T2R = std::atan(BOA * std::tan(P2R));
    auto const TM = (T1R + T2R) / 2.0f;
    auto const DTM = (T2R - T1R) / 2.0f;
    auto const STM = std::sin(TM);
    auto const CTM = std::cos(TM);
    auto const SDTM = std::sin(DTM);
    auto const CDTM = std::cos(DTM);
    auto const KL = STM * CDTM;
    auto const KK = SDTM * CTM;
    auto const SDLMR = std::sin(DLR / 2.0f);
    auto const L = SDTM * SDTM + SDLMR * SDLMR * (CDTM * CDTM - STM * STM);
    auto const CD = 1.0f - 2.0f * L;
    auto const DL = std::acos(CD);
    auto const SD = std::sin(DL);
    auto const T = DL / SD;
    auto const U = 2.0f * KL * KL / (1.0f - L);
    auto const V = 2.0f * KK * KK / L;
    auto const D = 4.0f * T * T;
    auto const X = U + V;
    auto const E = -2.0f * CD;
    auto const Y = U - V;
    auto const A = -D * E;
    auto const dist = AL * SD *
                      (T - (F / 4.0f) * (T * X - Y) +
                       FF64 * (X * (A + (T - (A + E) / 2.0f) * X) +
                               Y * (-2.0f * D + E * Y) + D * X * Y)) /
                      1000.0f;
    auto const TDLPM = std::tan(
        (DLR + (-((E * (4.0f - X) + 2.0f * Y) *
                  ((F / 2.0f) * T + FF64 * (32.0f * T + (A - 20.0f * T) * X -
                                            2.0f * (D + 2.0f) * Y)) /
                  4.0f) *
                std::tan(DLR))) /
        2.0f);
    auto const HAPBR = std::atan2(SDTM, (CTM * TDLPM));
    auto const HAMBR = std::atan2(CDTM, (STM * TDLPM));

    // This should be the net effect of the somewhat gnarly goto loops
    // in the original Fortran. Even as a former 370 assembler guy, ew,
    // just...ew.

    auto A1M2 = TAU + HAMBR - HAPBR;
    while (A1M2 < 0.0f || A1M2 >= TAU) {
        if (A1M2 < 0.0f)
            A1M2 += TAU;
        else if (A1M2 >= TAU)
            A1M2 -= TAU;
    }

    return std::make_pair(360.0f - (A1M2 / D2R), dist);
}
} // namespace

/******************************************************************************/
// Local Utilities
/******************************************************************************/

namespace {
// Displayable units. No need to translate these; the SI units are
// universal, and the standard units are only used in English.

constexpr QStringView UNITS_KM = u"km";
constexpr QStringView UNITS_MI = u"mi";

// In the spirit of the Fortran NINT() function, round and convert the
// provided floating-point value to an integer, for display purposes.

template <std::floating_point T> auto nint(T const value) {
    return static_cast<int>(std::round(value));
}
} // namespace

/******************************************************************************/
// Public Implementation
/******************************************************************************/

namespace Geodesic {
// Return as a compass direction, i.e., directional arrow and cardinal
// direction, or an empty string view if invalid.

QStringView Azimuth::compass() const {
    return isValid() ? COMPASS[static_cast<int>((m_value + 22.5f) / 45.0f) % 8]
                     : QStringView{};
}

// Return azimuth as a numeric string, to the nearest whole degree.
// If the caller requests units, we'll append a degree symbol.

QString Azimuth::toString(bool const units) const {
    if (!isValid())
        return QString{};

    return units ? QString("%1°").arg(nint(m_value))
                 : QString::number(nint(m_value));
}

// Return distance as a numeric string, to the nearest whole kilometer
// or mile. If we're close and either of the grids that gave rise to us
// to us was only of square magnitude, rather than subsquare or extended,
// prepend a '<' to indicate that we're close, but we're not sure just
// how close, and the actual distance is somewhere within the value.
//
// If the caller requests units, we'll append them.

QString Distance::toString(bool const miles, bool const units) const {
    if (!isValid())
        return QString{};

    auto value = isClose() ? CLOSE : m_value;
    if (miles)
        value /= 1.609344f;

    if (units && isClose())
        return QString("<%1 %2")
            .arg(nint(value))
            .arg(miles ? UNITS_MI : UNITS_KM);
    else if (units)
        return QString("%1 %2")
            .arg(nint(value))
            .arg(miles ? UNITS_MI : UNITS_KM);
    else if (isClose())
        return QString("<%1").arg(nint(value));
    else
        return QString::number(nint(value));
}

// The azdist() function is frankly something you don't want to run more
// than you have to. Additionally, while our contract defines the ability
// to compute a vector between any two valid grid identifiers, the fact is
// that the origin is going to be, practically speaking, always the local
// station.
//
// Vectors get looked up a lot, so caching them is of benefit. We use a
// two-level cache, the first level being a cache of origins, which will
// be reasonably persistent, the second level being an ephemeral cache of
// remotes.
//
// We're taking the default cache cost values of 100 here, so, rough math
// for the worst-case storage requirement here is:
//
//   - Keys are strings, and they're all short, so we'll see a
//     short string optimization, about 24 bytes per key.
//
//   - Vectors are the size of 2 floats, so 8 bytes per Vector.
//
//   - Key and associated Vector is therefore about 32 bytes.
//
//   - 100 elements per cache = 100 * 32 = 3.2K
//
//   - 100 caches of caches = ~3.2K * 100 = 320K. However, that's
//     worst case in the extreme; likely we'll never see more than
//     one origin, so likely only 3.2K total.
//
//   - Some overhead on top of that, but relatively speaking, not
//     much. We're therefore spending on the order of 4K of memory
//     here, along with some light integer math, in order to avoid
//     spending a lot of time repeating identical floating point
//     calculations.
//
// Note that the vector returned to the caller is theirs; it's always a
// copy of a cached version, or a new one that we create. They should be
// only 8 bytes in size (2 floats); so this should be very efficient; in
// theory, these return in a single 64-bit register.
//
// This function is reentrant, but practically speaking, it'd be unusual
// for this to be called from anything other than the GUI thread.

Vector vector(QStringView const origin, QStringView const remote) {
    using Cache = QCache<QString, Vector>;

    static QMutex mutex;
    static QCache<QString, Cache> caches;

    QMutexLocker lock(&mutex);

    // Caller is expected to hand us a lot of garbage; it's literally the
    // common case. Prior to getting too far into the weeds here, a quick
    // sanity check that what we've been handed could be expected to work.
    // If not, return a vector with invalid azimuth and invalid distance.

    if (!Maidenhead::valid(origin.trimmed()) ||
        !Maidenhead::valid(remote.trimmed())) {
        return Vector();
    }

    // Input data validated; we have a winner here; at this point we are
    // going to return a valid vector; get the data by which to create it.

    auto const data = normalize(origin, remote);

    // Perform first-level cache lookup; we should practically always hit
    // on this, other than the first time we're invoked.

    if (auto cache = caches.object(data.origin)) {
        // We've hit on the first level cache; if we hit on the second, then
        // return a copy of the cached vector to the caller, and we're outta
        // here. If we miss, create a vector, store a copy of it in the cache,
        // and return the original to the caller.

        if (auto const value = cache->object(data.remote)) {
            return *value;
        } else {
            auto const vector = Vector(azdist(data), data.square);
            cache->insert(data.remote, new Vector(vector));
            return vector;
        }
    }

    // We missed on the first-level cache; first time here for this origin.
    // Create a new second-level cache and a vector, storing a copy of the
    // vector in the cache, and then cache the cache. Return the original
    // vector to the caller.

    auto cache = new Cache();
    auto const vector = Vector(azdist(data), data.square);

    cache->insert(data.remote, new Vector(vector));
    caches.insert(data.origin, cache);

    return vector;
}
} // namespace Geodesic

/******************************************************************************/
