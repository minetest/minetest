--
-- Minimal Development Test
-- Mod: test
--

-- Try out PseudoRandom
pseudo = PseudoRandom(13)
assert(pseudo:next() == 22290)
assert(pseudo:next() == 13854)

-- Try out SHA1
sha = SHA1()
sha:add_bytes("Minetest")
assert(sha:get_hexdigest() == "882641ff539af9b262f8653db4d2f7719996364b")

