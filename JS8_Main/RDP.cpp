#include "RDP.h"
#include <cmath>
#include <utility>

/******************************************************************************/
// Implementation
/******************************************************************************/

// We'll typically end up with a ton of points to draw for the spectrum,
// and some simplification is worthwhile; use the Ramer–Douglas–Peucker
// algorithm to reduce to a smaller number of points.
//
// We'll modify the inbound polygon in place, such that anything we want
// to keep is at the start of the polygon and anything we want to omit
// is at the end, returning an iterator to the new end, i.e., the point
// one past the last point we want to keep.
//
// Our goal here is to avoid reallocations. Since we're at worst going to
// be leaving this the same size, we should be able to work with what we
// have already.
//
// Note that this is a functor; it's serially reusable, but not reentrant.
// Call it from one thread only. In practical use, that's not expected to
// be a problem, and it allows us to reuse allocated memory in a serial
// manner, rather than requesting it and freeing it constantly.

QPolygonF::iterator RDP::operator()(QPolygonF &polygon, qreal const epsilon) {
    // There's no point in proceeding with less than 3 points.

    if (polygon.size() < 3)
        return polygon.end();

    // We're always going to keep the first and last points; all others are
    // initially in play. Prime the stack with the full span; run the stack
    // machine until it empties.

    array.clear();
    array.resize(polygon.size());
    array.setBit(0);
    array.setBit(polygon.size() - 1);
    stack.push({0, polygon.size() - 1});

    while (!stack.isEmpty()) {
        auto const [index1, index2] = stack.pop();

        // Create a theoretical line between the first and last points
        // in the span we're presently considering; compute the vector
        // components and the line length.

        auto const &p1 = polygon[index1];
        auto const &p2 = polygon[index2];
        auto const dx = p2.x() - p1.x();
        auto const dy = p2.y() - p1.y();
        auto const ll = std::hypot(dx, dy);

        // Find the point within the span at the largest perpendicular
        // distance from the line greater than epsilon, if any.

        qreal limit = epsilon;
        qsizetype index = 0;

        for (auto i = index1 + 1; i < index2; ++i) {
            auto const &pi = polygon[i];
            auto const pd =
                std::abs(dy * (pi.x() - p1.x()) - dx * (pi.y() - p1.y())) / ll;
            if (pd > limit) {
                limit = pd;
                index = i;
            }
        }

        // If index is non-zero, that's our point. Keep it and break the
        // span into two spans at it, then continue working the problem.

        if (index) {
            array.setBit(index);
            stack.push({index1, index});
            stack.push({index, index2});
        }
    }

    // Our array now contains bits set to true for every point that
    // should be kept, false for those that should be removed. Move
    // everything we want to keep to the front and return the first
    // element to remove.

    auto first = polygon.begin();

    for (qsizetype i = 0; i < polygon.size(); ++i) {
        if (array.at(i))
            *first++ = std::move(polygon[i]);
    }

    return first;
}

/******************************************************************************/
