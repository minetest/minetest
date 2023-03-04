/*
Minetest
Copyright (C) 2023 DS

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "test.h"

#include "util/pointer.h"
#include <array>

class TestPointer : public TestBase
{
public:
	TestPointer() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestPointer"; }

	void runTests(IGameDef *gamedef);

	void testUniqueBuffer();
	void testSharedBuffer();
	void testView();
	void testConstView();

private:
	struct A {
		int a;
		int b;
		A() = default;
		A(int x, int y) : a(x), b(y) {}
	};
};

static TestPointer g_test_instance;

void TestPointer::runTests(IGameDef *gamedef)
{
	TEST(testUniqueBuffer);
	TEST(testSharedBuffer);
	TEST(testView);
	TEST(testConstView);
}

////////////////////////////////////////////////////////////////////////////////

void TestPointer::testUniqueBuffer()
{
	UniqueBuffer<A> ubuf1 = make_buffer<A>(12);
	UniqueBuffer<A> ubuf2 = make_buffer_for_overwrite<A>(42);
	UniqueBuffer<A> ubuf3;
	UniqueBuffer<A> ubuf4 = make_buffer_for_overwrite<A>(0);
	ubuf1.at(2) = A(4, 5);
	ubuf1.at(0);
	ubuf1.at(11);
	UASSERT(ubuf1);
	UASSERT(ubuf2);
	UASSERT(!ubuf3);
	UASSERT(!ubuf4);

	ubuf3 = std::move(ubuf2);
	UASSERT(ubuf3);
	UASSERT(!ubuf2);
	UASSERT(ubuf3.size() == 42);
	UASSERT(ubuf2.size() == 0);

	std::unique_ptr<A[]> up1 = ubuf3.release();
	std::unique_ptr<A[]> up2 = make_unique_for_overwrite<A[]>(13);
	std::unique_ptr<A> up3 = make_unique_for_overwrite<A>();
	std::unique_ptr<A> up4 = std::make_unique<A>(1, 2);
	UASSERT(!ubuf3);
	UASSERT(ubuf3.size() == 0);

	ubuf2.reset(std::move(up1), 42);
	UASSERT(!up1);
	UASSERT(ubuf2);
	UASSERT(ubuf2.size() == 42);
	UASSERT(ubuf2.get());

	ubuf2.copyTo(ubuf3);
	ubuf3.copyTo(ubuf2);
	ubuf1 = ubuf3.copy();
	UASSERT(ubuf1.size() == 42);
}

void TestPointer::testSharedBuffer()
{
	UniqueBuffer<A> ubuf1 = make_buffer<A>(14);
	SharedBuffer<A> sbuf1 = make_buffer<A>(13);
	SharedBuffer<A> sbuf2 = make_buffer_for_overwrite<A>(42);
	SharedBuffer<A> sbuf3 = std::move(ubuf1);
	SharedBuffer<A> sbuf4;
	SharedBuffer<A> sbuf5 = make_buffer<A>(0);
	sbuf2[0];
	sbuf3.at(13);
	UASSERT(sbuf1);
	UASSERT(sbuf1.get());
	UASSERT(sbuf1.size() == 13);
	UASSERT(!sbuf1.empty());
	UASSERT(sbuf3);
	UASSERT(sbuf3.size() == 14);
	UASSERT(ubuf1.empty());
	UASSERT(!ubuf1);
	UASSERT(sbuf5.empty());
	UASSERT(!sbuf5);
	UASSERT(!sbuf5.get());

	sbuf4 = sbuf3;
	UASSERT(sbuf4.get() == sbuf3.get());

	sbuf4 = std::move(sbuf3);
	UASSERT(!sbuf3);
	UASSERT(sbuf4);

	sbuf2.reset();
	UASSERT(sbuf2.empty());

	sbuf2 = sbuf4.moveOut();
	UASSERT(!sbuf4);

	sbuf4 = sbuf2;
	sbuf5 = sbuf4.moveOrCopyOut();
	UASSERT(sbuf5.get() != sbuf2.get());
	UASSERT(!sbuf4);
	UASSERT(sbuf5);
	UASSERT(sbuf2);
}

void TestPointer::testView()
{
	std::array<A, 4> arr1;
	UniqueBuffer<A> ubuf1 = make_buffer<A>(13);
	UniqueBuffer<A> ubuf2;
	SharedBuffer<A> sbuf1 = make_buffer<A>(14);

	View<A> v1;
	View<A> v2 = nullptr;
	View<A> v3 = {&arr1[0], arr1.size()};
	View<A> v4(&arr1[0], arr1.size());
	View<A> v5 = {arr1.begin(), arr1.end()};
	View<A> v6 = ubuf1;
	View<A> v7 = sbuf1;
	View<A> v8 = {arr1.data(), 1};
	View<A> v9 = {arr1.data(), 0};
	View<A> v10 = ubuf2;
	View<A> v11 = v3;
	View<A> v12 = std::move(v3);
	v7.at(13);
	UASSERT(!v1);
	UASSERT(v1.empty());
	UASSERT(!v1.get());
	UASSERT(v1.size() == 0);
	UASSERT(!v2);
	UASSERT(v3.get() == arr1.data());
	UASSERT(v3.size() == 4);
	UASSERT(!v3.empty());
	UASSERT(v4.size() == 4);
	UASSERT(v4.get() == v3.get());
	UASSERT(v5.size() == 4);
	UASSERT(v5.get() == v3.get());
	UASSERT(v6.get() == ubuf1.get());
	UASSERT(v6.size() == ubuf1.size());
	UASSERT(v7.get() == sbuf1.get());
	UASSERT(v7.size() == sbuf1.size());
	UASSERT(v8.get() == arr1.data());
	UASSERT(v8.size() == 1);
	UASSERT(v9.get() == arr1.data());
	UASSERT(v9.size() == 0);
	UASSERT(v9.empty());
	UASSERT(v9);
	UASSERT(!v10);
	UASSERT(v10.size() == 0);
	UASSERT(v11.get() == arr1.data());
	UASSERT(v11.size() == 4);
	UASSERT(v12.get() == arr1.data());
	UASSERT(v12.size() == 4);

	v11 = v3;
	v12 = std::move(v3);
	UASSERT(v3.get() == arr1.data());
	UASSERT(v3.size() == 4);
	UASSERT(v11.get() == arr1.data());
	UASSERT(v11.size() == 4);
	UASSERT(v12.get() == arr1.data());
	UASSERT(v12.size() == 4);

	v6.reset();
	v7.reset(arr1.data(), 2);
	v8.reset(v8.begin()+1, v8.end());
	ubuf2 = v3.copy();
	UASSERT(!v6);
	UASSERT(v6.empty());
	UASSERT(v7.get() == arr1.data());
	UASSERT(v7.size() == 2);
	UASSERT(v8.get() == &arr1[1]);
	UASSERT(v8.size() == 0);
	UASSERT(v8.get() == &arr1[1]);
	UASSERT(v8.size() == 0);
	UASSERT(ubuf2);
	UASSERT(ubuf2.size() == 4);
}

void TestPointer::testConstView()
{
	UniqueBuffer<A> ubuf1 = make_buffer<A>(13);
	UniqueBuffer<A> ubuf2;
	UniqueBuffer<const A> cubuf1 = make_buffer<A>(14);
	SharedBuffer<A> sbuf1 = make_buffer<A>(15);
	SharedBuffer<const A> csbuf1;
	View<A> v1 = ubuf1;
	ConstView<A> cv1 = v1;
	UASSERT(v1.get() == cv1.get());

	ubuf2 = cv1.copy();
	csbuf1 = sbuf1;
	UASSERT(ubuf2);
	UASSERT(csbuf1.get() == sbuf1.get());
}
