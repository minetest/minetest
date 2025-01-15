// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "catch.h"
#include "irrMath.h"
#include "matrix4.h"
#include "quaternion.h"
#include "irr_v3d.h"

using matrix4 = core::matrix4;

static bool matrix_equals(const matrix4 &a, const matrix4 &b) {
    return a.equals(b, 0.00001f);
}

TEST_CASE("quaternion") {

// Make sure that the conventions are consistent
SECTION("equivalence to euler rotations") {
    auto test_rotation = [](v3f rad) {
        matrix4 R;
        R.setRotationRadians(rad);
        v3f rad2;
        core::quaternion(rad).toEuler(rad2);
        matrix4 R2;
        R2.setRotationRadians(rad2);
        CHECK(matrix_equals(R, R2));
    };

    test_rotation({100, 200, 300});
    Catch::Generators::RandomFloatingGenerator<f32> gen(0.0f, 2 * core::PI, Catch::getSeed());
    for (int i = 0; i < 1000; ++i)
        test_rotation(v3f{gen.get(), gen.get(), gen.get()});
    for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
    for (int k = 0; k < 4; k++)
        test_rotation(core::PI / 4.0f * v3f(i, j, k));
}

}