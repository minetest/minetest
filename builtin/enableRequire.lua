-- enables a sensible require system instead of that dofile nonsense

-- tracks which files are loaded and evaluated not reloading or
-- reevaluating them respectively unless explicitly asked to.

-- uses the actual directory separator,
-- though / should always work even in 'doze

-- automatically adds the '.lua' suffix. 

-- there is a loadResource function if that behavior is undesired
-- but nothing should be cached in loaded or evaluated
-- tables, except lua code.

if minetest.require == nil then
   local loaded = {}
   local evaluated = {}

   local DIR_SEPARATOR = package.config:sub(1,1)

   local function minetestLoadResource(module,what)
      local path = minetest.get_modpath(module)
      if path == nil then
         error("Could not find module "..module)
      end
      path = path..DIR_SEPARATOR..what
      local handle,err = io.open(path,"r")
      if handle == nil then
         error(err)
      end
      return handle
   end

   local function minetestRequire(module,name)
      local key = module..'/'..name
      local gotcha = evaluated[key]
      if gotcha ~= nil then
         return gotcha
      end
      gotcha = loaded[key]
      if gotcha ~= nil then
         gotcha = gotcha()
         evaluated[key] = gotcha
         return gotcha
      end
      handle = minetestLoadResource(module,name..'.lua')
      local contents = handle:read("*a")   
      handle:close()
      gotcha = assert(loadstring(contents,module.."/"..name))
      loaded[key] = gotcha
      gotcha = gotcha()
      evaluated[key] = gotcha
      return gotcha
   end

   local function minetestUninit(module,name)
      local key = module..'/'..name
      evaluated[key] = nil
   end
   
   local function minetestReinit(module,name)
      minetestUninit(module,name)
      return minetestRequire(module,name)
   end

   local function minetestUnload(module,name)
      local key = module..'/'..name
      evaluated[key] = nil
      loaded[key] = nil
   end

   local function minetestReload(module,name)
      minetestUnload(module,name)
      return minetestRequire(module,name)
   end

   minetest.require = minetestRequire
   minetest.requiretools = {
      loadResource = minetestLoadResource,
      uninit = minetestUninit,
      reinit = minetestReinit,
      unload = minetestUnload,
      reload = minetestReload
   }
end