#ifndef OSMIUM_GEOM_TILE_HPP
#define OSMIUM_GEOM_TILE_HPP

/*

This file is part of Osmium (https://osmcode.org/libosmium).

Copyright 2013-2021 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <osmium/geom/coordinates.hpp>
#include <osmium/geom/mercator_projection.hpp>
#include <osmium/osm/location.hpp>

#include <cassert>
#include <cstdint>

namespace osmium {

    namespace geom {

        namespace detail {

            template <typename T>
            inline constexpr const T& clamp(const T& value, const T& min, const T& max) {
                return value < min ? min : (max < value ? max : value);
            }

        } // namespace detail

        /**
         * Returns the number of tiles (in each direction) for the given zoom
         * level.
         */
        inline constexpr uint32_t num_tiles_in_zoom(uint32_t zoom) noexcept {
            return 1U << zoom;
        }

        /**
         * Returns the width or height of a tile in web mercator coordinates for
         * the given zoom level.
         */
        inline constexpr double tile_extent_in_zoom(uint32_t zoom) noexcept {
            return detail::max_coordinate_epsg3857 * 2 / num_tiles_in_zoom(zoom);
        }

        /**
         * Get the tile x number from an x coordinate in web mercator
         * projection in the given zoom level. Tiles are numbered from left
         * to right.
         */
        inline constexpr uint32_t mercx_to_tilex(uint32_t zoom, double x) noexcept {
            return static_cast<uint32_t>(detail::clamp<int32_t>(
                static_cast<int32_t>((x + detail::max_coordinate_epsg3857) / tile_extent_in_zoom(zoom)),
                0, num_tiles_in_zoom(zoom) - 1));
        }

        /**
         * Get the tile y number from an y coordinate in web mercator
         * projection in the given zoom level. Tiles are numbered from top
         * to bottom.
         */
        inline constexpr uint32_t mercy_to_tiley(uint32_t zoom, double y) noexcept {
            return static_cast<uint32_t>(detail::clamp<int32_t>(
                static_cast<int32_t>((detail::max_coordinate_epsg3857 - y) / tile_extent_in_zoom(zoom)),
                0, num_tiles_in_zoom(zoom) - 1));
        }

        /**
         * A tile in the usual Mercator projection.
         */
        struct Tile {

            enum {
                max_zoom = 30U
            };

            /// x coordinate
            uint32_t x;

            /// y coordinate
            uint32_t y;

            /// Zoom level
            uint32_t z;

            /**
             * Create a tile with the given zoom level and x and y tile
             * coordinates.
             *
             * The values are not checked for validity.
             *
             * @pre @code zoom <= 30 && x < 2^zoom && y < 2^zoom @endcode
             */
            explicit Tile(uint32_t zoom, uint32_t tx, uint32_t ty) noexcept :
                x(tx),
                y(ty),
                z(zoom) {
                assert(zoom <= max_zoom);
                assert(x < num_tiles_in_zoom(zoom));
                assert(y < num_tiles_in_zoom(zoom));
            }

            /**
             * Create a tile with the given zoom level that contains the given
             * location.
             *
             * The values are not checked for validity.
             *
             * @pre @code location.valid() && zoom <= 30 @endcode
             */
            explicit Tile(uint32_t zoom, const osmium::Location& location) :
                z(zoom) {
                assert(zoom <= max_zoom);
                assert(location.valid());
                const auto coordinates = lonlat_to_mercator(location);
                x = mercx_to_tilex(zoom, coordinates.x);
                y = mercy_to_tiley(zoom, coordinates.y);
            }

            /**
             * Create a tile with the given zoom level that contains the given
             * coordinates in Mercator projection.
             *
             * The values are not checked for validity.
             *
             * @pre @code coordinates.valid() && zoom <= 30 @endcode
             */
            explicit Tile(uint32_t zoom, const osmium::geom::Coordinates& coordinates) :
                z(zoom) {
                assert(zoom <= max_zoom);
                x = mercx_to_tilex(zoom, coordinates.x);
                y = mercy_to_tiley(zoom, coordinates.y);
            }

            /**
             * Check whether this tile is valid. For a tile to be valid the
             * zoom level must be between 0 and 30 and the coordinates must
             * each be between 0 and 2^zoom-1.
             */
            bool valid() const noexcept {
                if (z > max_zoom) {
                    return false;
                }
                const auto max = num_tiles_in_zoom(z);
                return x < max && y < max;
            }

        }; // struct Tile

        /// Tiles are equal if all their attributes are equal.
        inline bool operator==(const Tile& lhs, const Tile& rhs) noexcept {
            return lhs.z == rhs.z && lhs.x == rhs.x && lhs.y == rhs.y;
        }

        inline bool operator!=(const Tile& lhs, const Tile& rhs) noexcept {
            return !(lhs == rhs);
        }

        /**
         * This defines an arbitrary order on tiles for use in std::map etc.
         */
        inline bool operator<(const Tile& lhs, const Tile& rhs) noexcept {
            if (lhs.z < rhs.z) {
                return true;
            }
            if (lhs.z > rhs.z) {
                return false;
            }
            if (lhs.x < rhs.x) {
                return true;
            }
            if (lhs.x > rhs.x) {
                return false;
            }
            return lhs.y < rhs.y;
        }

    } // namespace geom

} // namespace osmium

#endif // OSMIUM_GEOM_TILE_HPP
