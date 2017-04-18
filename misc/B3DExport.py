#!BPY

"""
Name: 'B3D Exporter (.b3d)...'
Blender: 259
Group: 'Export'
Tooltip: 'Export to Blitz3D file format (.b3d)'
"""
__author__ = ["iego 'GaNDaLDF' Parisi, MTLZ (is06), Joerg Henrichs, Marianne Gagnon"]
__url__ = ["www.gandaldf.com"]
__version__ = "3.0"
__bpydoc__ = """\
"""

# BLITZ3D EXPORTER 3.0
# Copyright (C) 2009 by Diego "GaNDaLDF" Parisi  -  www.gandaldf.com
# Lightmap issue fixed by Capricorn 76 Pty. Ltd. - www.capricorn76.com
# Blender 2.63 compatiblity based on work by MTLZ, www.is06.com
# With changes by Marianne Gagnon and Joerg Henrichs, supertuxkart.sf.net (Copyright (C) 2011-2012)
#
# LICENSE:
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

bl_info = {
    "name": "B3D (BLITZ3D) Model Exporter",
    "description": "Exports a blender scene or object to the B3D (BLITZ3D) format",
    "author": "Diego 'GaNDaLDF' Parisi, MTLZ (is06), Joerg Henrichs, Marianne Gagnon",
    "version": (3,1),
    "blender": (2, 5, 9),
    "api": 31236,
    "location": "File > Export",
    "warning": '', # used for warning icon and text in addons panel
    "wiki_url": "http://supertuxkart.sourceforge.net/Get_involved",
    "tracker_url": "https://sourceforge.net/apps/trac/supertuxkart/",
    "category": "Import-Export"}


import bpy
import sys,os,os.path,struct,math,string
import mathutils
import math

if not hasattr(sys,"argv"): sys.argv = ["???"]


#Global Stacks
b3d_parameters = {}
texture_flags  = []
texs_stack     = {}
brus_stack     = []
vertex_groups  = []
bone_stack     = {}
keys_stack     = []

texture_count = 0

# bone_stack indices constants
BONE_PARENT_MATRIX = 0
BONE_PARENT = 1
BONE_ITSELF = 2

# texture stack indices constants
TEXTURE_ID = 0
TEXTURE_FLAGS = 1

per_face_vertices = {}

the_scene = None

#Transformation Matrix
TRANS_MATRIX = mathutils.Matrix([[1,0,0,0],[0,0,1,0],[0,1,0,0],[0,0,0,1]])
BONE_TRANS_MATRIX = mathutils.Matrix([[-1,0,0,0],[0,0,-1,0],[0,-1,0,0],[0,0,0,1]])

DEBUG = False
PROGRESS = True
PROGRESS_VERBOSE = False

tesselated_objects = {}

#Support Functions
def write_int(value):
    return struct.pack("<i",value)

def write_float(value):
    return struct.pack("<f",value)

def write_float_couple(value1, value2):
    return struct.pack("<ff", value1, value2)

def write_float_triplet(value1, value2, value3):
    return struct.pack("<fff", value1, value2, value3)

def write_float_quad(value1, value2, value3, value4):
    return struct.pack("<ffff", value1, value2, value3, value4)
    
def write_string(value):
    binary_format = "<%ds"%(len(value)+1)
    return struct.pack(binary_format, str.encode(value))

def write_chunk(name,value):
    dummy = bytearray()
    return dummy + name + write_int(len(value)) + value

trimmed_paths = {}

def getArmatureAnimationEnd(armature):
    end_frame = 1
    if armature.animation_data.action:
        ipo = armature.animation_data.action.fcurves
        for curve in ipo:
            if "pose" in curve.data_path:
                end_frame = max(end_frame, curve.keyframe_points[-1].co[0])
    
    for nla_track in armature.animation_data.nla_tracks:
        if len(nla_track.strips) > 0:
            end_frame = max(end_frame, nla_track.strips[-1].frame_end)
        
    return end_frame

# ==== Write B3D File ====
# (main exporter function)
def write_b3d_file(filename, objects=[]):
    global texture_flags, texs_stack, trimmed_paths, tesselated_objects
    global brus_stack, vertex_groups, bone_stack, keys_stack

    #Global Stacks
    texture_flags = []
    texs_stack = {}
    brus_stack = []
    vertex_groups = []
    bone_stack = []
    keys_stack = []
    trimmed_paths = {}
    file_buf = bytearray()
    temp_buf = bytearray()
    tesselated_objects = {}

    import time
    start = time.time()

    temp_buf += write_int(1) #Version
    temp_buf += write_texs(objects) #TEXS
    temp_buf += write_brus(objects) #BRUS
    temp_buf += write_node(objects) #NODE

    if len(temp_buf) > 0:
        file_buf += write_chunk(b"BB3D",temp_buf)
        temp_buf = ""

    file = open(filename,'wb')
    file.write(file_buf)
    file.close()
    
    # free memory
    trimmed_paths = {}
    
    end = time.time()
    
    print("Exported in", (end - start))


def tesselate_if_needed(objdata):
    if objdata not in tesselated_objects:
        objdata.calc_tessface()
        tesselated_objects[objdata] = True
    return objdata

def getUVTextures(obj_data):
    # BMesh in blender 2.63 broke this
    if bpy.app.version[1] >= 63:
        return tesselate_if_needed(obj_data).tessface_uv_textures
    else:
        return obj_data.uv_textures

def getFaces(obj_data):
    # BMesh in blender 2.63 broke this
    if bpy.app.version[1] >= 63:
        return tesselate_if_needed(obj_data).tessfaces
    else:
        return obj_data.faces

def getVertexColors(obj_data):
    # BMesh in blender 2.63 broke this
    if bpy.app.version[1] >= 63:
        return tesselate_if_needed(obj_data).tessface_vertex_colors
    else:
        return obj_data.vertex_colors

# ==== Write TEXS Chunk ====
def write_texs(objects=[]):
    global b3d_parameters
    global trimmed_paths
    global texture_count
    texs_buf = bytearray()
    temp_buf = bytearray()
    layer_max = 0
    obj_count = 0
    set_wrote = 0

    if objects:
        exp_obj = objects
    else:
        if b3d_parameters.get("export-selected"):
            exp_obj = [ob for ob in bpy.data.objects if ob.select]
        else:
            exp_obj = bpy.data.objects

    if PROGRESS: print(len(exp_obj),"TEXS")

    if PROGRESS_VERBOSE: progress = 0

    for obj in exp_obj:
        
        if PROGRESS_VERBOSE:
            progress = progress + 1
            if (progress % 10 == 0): print("TEXS",progress,"/",len(exp_obj))
        
        if obj.type == "MESH":
            set_count = 0
            set_wrote = 0
            #data = obj.getData(mesh = True)
            data = obj.data
            
            # FIXME?
            #orig_uvlayer = data.activeUVLayer
            
            layer_set = [[],[],[],[],[],[],[],[]]
            
            # 8 UV layers are supported
            texture_flags.append([None,None,None,None,None,None,None,None])

            #if len(data.getUVLayerNames()) <= 8:
            uv_textures = getUVTextures(data)
            if len(uv_textures) <= 8:
                if len(uv_textures) > layer_max:
                    layer_max = len(uv_textures)
            else:
                layer_max = 8

            for face in getFaces(data):
                for iuvlayer,uvlayer in enumerate(uv_textures):
                    if iuvlayer < 8:
                        
                        # FIXME?
                        #data.activeUVLayer = uvlayer
                        
                        #layer_set[iuvlayer].append(face.uv)
                        new_data = None
                        try:
                            new_data = uvlayer.data[face.index].uv
                        except:
                            pass
                        
                        layer_set[iuvlayer].append( new_data )

            for i in range(len(uv_textures)):
                if set_wrote:
                    set_count += 1
                    set_wrote = 0

                for iuvlayer in range(i,len(uv_textures)):
                    if layer_set[i] == layer_set[iuvlayer]:
                        if texture_flags[obj_count][iuvlayer] is None:
                            if set_count == 0:
                                tex_flag = 1
                            elif set_count == 1:
                                tex_flag = 65536
                            elif set_count > 1:
                                tex_flag = 1
                            if b3d_parameters.get("mipmap"):
                                enable_mipmaps=8
                            else:
                                enable_mipmaps=0
                            texture_flags[obj_count][iuvlayer] = tex_flag | enable_mipmaps
                            set_wrote = 1

            for face in getFaces(data):
                for iuvlayer,uvlayer in enumerate(uv_textures):
                    if iuvlayer < 8:
                        
                        if not (iuvlayer < len(uv_textures)):
                            continue
                        
                        # FIXME?
                        #data.activeUVLayer = uvlayer
                        
                        #if DEBUG: print("<uv face=", face.index, ">")
                        
                        img = getUVTextures(data)[iuvlayer].data[face.index].image
                        
                        if img and b3d_parameters.get("export-textures"):
                            
                            if img.filepath in trimmed_paths:
                                img_name = trimmed_paths[img.filepath]
                            else:
                                img_name = bpy.path.basename(img.filepath)
                                trimmed_paths[img.filepath] = img_name

                            if not img_name in texs_stack:
                                texs_stack[img_name] = [len(texs_stack), texture_flags[obj_count][iuvlayer]]
                                temp_buf += write_string(img_name) #Texture File Name
                                temp_buf += write_int(texture_flags[obj_count][iuvlayer]) #Flags
                                temp_buf += write_int(2)   #Blend
                                temp_buf += write_float(0) #X_Pos
                                temp_buf += write_float(0) #Y_Pos
                                temp_buf += write_float(1) #X_Scale
                                temp_buf += write_float(1) #Y_Scale
                                temp_buf += write_float(0) #Rotation
                            #else:
                            #    if DEBUG: print("    <image id=(previous)","name=","'"+img_name+"'","/>")
                            
                        #if DEBUG: print("</uv>")

            obj_count += 1

            #FIXME?
            #if orig_uvlayer:
            #    data.activeUVLayer = orig_uvlayer

    texture_count = layer_max

    if len(temp_buf) > 0:
        texs_buf += write_chunk(b"TEXS",temp_buf)
        temp_buf = ""

    return texs_buf

# ==== Write BRUS Chunk ====
def write_brus(objects=[]):
    global b3d_parameters
    global trimmed_paths
    global texture_count
    brus_buf = bytearray()
    temp_buf = bytearray()
    mat_count = 0
    obj_count = 0

    if DEBUG: print("<!-- BRUS chunk -->")

    if objects:
        exp_obj = objects
    else:
        if  b3d_parameters.get("export-selected"):
            exp_obj = [ob for ob in bpy.data.objects if ob.select]
        else:
            exp_obj = bpy.data.objects

    if PROGRESS: print(len(exp_obj),"BRUS")
    if PROGRESS_VERBOSE: progress = 0

    for obj in exp_obj:
        
        if PROGRESS_VERBOSE:
            progress += 1
            if (progress % 10 == 0): print("BRUS",progress,"/",len(exp_obj))
            
        if obj.type == "MESH":
            data = obj.data
            
            uv_textures = getUVTextures(data)
            
            if len(uv_textures) <= 0:
                continue

            if DEBUG: print("<obj name=",obj.name,">")

            img_found = 0
            
            for face in getFaces(data):
                
                face_stack = []
                
                for iuvlayer,uvlayer in enumerate(uv_textures):
                    if iuvlayer < 8:
                        
                        img_id = -1
                        
                        if face.index >= len(uv_textures[iuvlayer].data):
                            continue
                        
                        img = uv_textures[iuvlayer].data[face.index].image
                        
                        if not img:
                            continue
                        
                        img_found = 1
                        
                        if img.filepath in trimmed_paths:
                            img_name = trimmed_paths[img.filepath]
                        else:
                            img_name = os.path.basename(img.filepath)
                            trimmed_paths[img.filepath] = img_name
                        
                        if DEBUG: print("    <!-- Building FACE 'stack' -->")
                        
                        if img_name in texs_stack:
                            img_id = texs_stack[img_name][TEXTURE_ID]
                        
                        face_stack.insert(iuvlayer,img_id)
                        if DEBUG: print("    <uv face=",face.index,"layer=", iuvlayer, " imgid=", img_id, "/>")

                for i in range(len(face_stack),texture_count):
                    face_stack.append(-1)


                if DEBUG: print("    <!-- Writing chunk -->")
                
                if not img_found:
                    if data.materials:
                        if data.materials[face.material_index]:
                            mat_data = data.materials[face.material_index]
                            mat_colr = mat_data.diffuse_color[0]
                            mat_colg = mat_data.diffuse_color[1]
                            mat_colb = mat_data.diffuse_color[2]
                            mat_alpha = mat_data.alpha
                            mat_name = mat_data.name

                            if not mat_name in brus_stack:
                                brus_stack.append(mat_name)
                                temp_buf += write_string(mat_name) #Brush Name
                                temp_buf += write_float(mat_colr)  #Red
                                temp_buf += write_float(mat_colg)  #Green
                                temp_buf += write_float(mat_colb)  #Blue
                                temp_buf += write_float(mat_alpha) #Alpha
                                temp_buf += write_float(0)         #Shininess
                                temp_buf += write_int(1)           #Blend
                                if b3d_parameters.get("vertex-colors") and len(getVertexColors(data)):
                                    temp_buf += write_int(2) #Fx
                                else:
                                    temp_buf += write_int(0) #Fx

                                for i in face_stack:
                                    temp_buf += write_int(i) #Texture ID
                    else:
                        if b3d_parameters.get("vertex-colors") and len(getVertexColors(data)) > 0:
                            if not face_stack in brus_stack:
                                brus_stack.append(face_stack)
                                mat_count += 1
                                temp_buf += write_string("Brush.%.3i"%mat_count) #Brush Name
                                temp_buf += write_float(1) #Red
                                temp_buf += write_float(1) #Green
                                temp_buf += write_float(1) #Blue
                                temp_buf += write_float(1) #Alpha
                                temp_buf += write_float(0) #Shininess
                                temp_buf += write_int(1)   #Blend
                                temp_buf += write_int(2)   #Fx

                                for i in face_stack:
                                    temp_buf += write_int(i) #Texture ID
                else: # img_found
                
                    if not face_stack in brus_stack:
                        brus_stack.append(face_stack)
                        mat_count += 1
                        temp_buf += write_string("Brush.%.3i"%mat_count) #Brush Name
                        temp_buf += write_float(1) #Red
                        temp_buf += write_float(1) #Green
                        temp_buf += write_float(1) #Blue
                        temp_buf += write_float(1) #Alpha
                        temp_buf += write_float(0) #Shininess
                        temp_buf += write_int(1)   #Blend
                        
                        if DEBUG: print("    <brush id=",len(brus_stack),">")
                        
                        if b3d_parameters.get("vertex-colors") and len(getVertexColors(data)) > 0:
                            temp_buf += write_int(2) #Fx
                        else:
                            temp_buf += write_int(0) #Fx

                        for i in face_stack:
                            temp_buf += write_int(i) #Texture ID
                            if DEBUG: print("        <texture id=",i,">")
                        
                        if DEBUG: print("    </brush>")
                
                if DEBUG: print("")

            if DEBUG: print("</obj>")
            obj_count += 1

            #FIXME?
            #if orig_uvlayer:
            #    data.activeUVLayer = orig_uvlayer

    if len(temp_buf) > 0:
        brus_buf += write_chunk(b"BRUS",write_int(texture_count) + temp_buf) #N Texs
        temp_buf = ""
    
    return brus_buf

# ==== Write NODE Chunk ====
def write_node(objects=[]):
    global bone_stack
    global keys_stack
    global b3d_parameters
    global the_scene
    
    root_buf = []
    node_buf = []
    main_buf = bytearray()
    temp_buf = []
    obj_count = 0
    amb_light = 0

    num_mesh = 0
    num_ligs = 0
    num_cams = 0
    num_lorc = 0
    #exp_scn = Blender.Scene.GetCurrent()
    #exp_scn = the_scene
    #exp_con = exp_scn.getRenderingContext()

    #first_frame = Blender.Draw.Create(exp_con.startFrame())
    #last_frame = Blender.Draw.Create(exp_con.endFrame())
    #num_frames = last_frame.val - first_frame.val
    first_frame = the_scene.frame_start

    if DEBUG: print("<node first_frame=", first_frame, ">")

    if objects:
        exp_obj = objects
    else:
        if b3d_parameters.get("export-selected"):
            exp_obj = [ob for ob in bpy.data.objects if ob.select]
        else:
            exp_obj = bpy.data.objects

    for obj in exp_obj:
        if obj.type == "MESH":
            num_mesh += 1
        if obj.type == "CAMERA":
            num_cams += 1
        if obj.type == "LAMP":
            num_ligs += 1

    if b3d_parameters.get("cameras"):
        num_lorc += num_cams

    if b3d_parameters.get("lights"):
        num_lorc += 1
        num_lorc += num_ligs

    if num_mesh + num_lorc > 1:
        exp_root = 1
    else:
        exp_root = 0

    if exp_root:
        root_buf.append(write_string("ROOT")) #Node Name

        root_buf.append(write_float_triplet(0, 0, 0)) #Position X,Y,Z
        root_buf.append(write_float_triplet(1, 1, 1)) #Scale X, Y, Z
        root_buf.append(write_float_quad(1, 0, 0, 0)) #Rotation W, X, Y, Z

    if PROGRESS: progress = 0

    for obj in exp_obj:
        
        if PROGRESS:
            progress += 1
            print("NODE:",progress,"/",len(exp_obj))
        
        if obj.type == "MESH":
            
            if DEBUG: print("    <mesh name=",obj.name,">")
            
            bone_stack = {}
            keys_stack = []

            anim_data = None
            
            # check if this object has an armature modifier
            for curr_mod in obj.modifiers:
                if curr_mod.type == 'ARMATURE':
                    arm = curr_mod.object
                    if arm is not None:
                        anim_data = arm.animation_data

            # check if this object has an armature parent (second way to do armature animations in blender)
            if anim_data is None:
                if obj.parent:
                    if obj.parent.type == "ARMATURE":
                        arm = obj.parent
                        if arm.animation_data:
                            anim_data = arm.animation_data

            if anim_data:
                matrix = mathutils.Matrix()

                temp_buf.append(write_string(obj.name)) #Node Name
                
                position = matrix.to_translation()
                temp_buf.append(write_float_triplet(position[0], position[1], position[2])) #Position X, Y, Z

                scale = matrix.to_scale()
                temp_buf.append(write_float_triplet(scale[0], scale[2], scale[1])) #Scale X, Y, Z

                if DEBUG: print("        <arm name=", obj.name, " loc=", -position[0], position[1], position[2], " scale=", scale[0], scale[1], scale[2], "/>")
                
                quat = matrix.to_quaternion()
                quat.normalize()

                temp_buf.append(write_float_quad(quat.w, quat.x, quat.z, quat.y))
            else:
                if b3d_parameters.get("local-space"):
                    matrix = TRANS_MATRIX.copy()
                    scale_matrix = mathutils.Matrix()
                else:
                    matrix = obj.matrix_world*TRANS_MATRIX
                    scale_matrix = obj.matrix_world.copy()
                
                
                if bpy.app.version[1] >= 62:
                    # blender 2.62 broke the API : Column-major access was changed to row-major access
                    tmp = mathutils.Vector([matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1]])
                    matrix[0][1] = matrix[0][2]
                    matrix[1][1] = matrix[1][2]
                    matrix[2][1] = matrix[2][2]
                    matrix[3][1] = matrix[3][2]
                    
                    matrix[0][2] = tmp[0]
                    matrix[1][2] = tmp[1]
                    matrix[2][2] = tmp[2]
                    matrix[3][2] = tmp[3]
                else:
                    tmp = mathutils.Vector(matrix[1])
                    matrix[1] = matrix[2]
                    matrix[2] = tmp

                temp_buf.append(write_string(obj.name)) #Node Name

                #print("Matrix : ", matrix)
                position = matrix.to_translation()

                temp_buf.append(write_float_triplet(position[0], position[2], position[1])) 

                scale = scale_matrix.to_scale()
                temp_buf.append(write_float_triplet(scale[0], scale[2], scale[1]))

                quat = matrix.to_quaternion()
                quat.normalize()

                temp_buf.append(write_float_quad(quat.w, quat.x, quat.z, quat.y))
                  
                if DEBUG:
                    print("        <position>",position[0],position[2],position[1],"</position>")
                    print("        <scale>",scale[0],scale[1],scale[2],"</scale>")
                    print("        <rotation>", quat.w, quat.x, quat.y, quat.z, "</rotation>")
            
            if anim_data:
                the_scene.frame_set(1,subframe=0.0)
                
                arm_matrix = arm.matrix_world
                
                if b3d_parameters.get("local-space"):
                    arm_matrix = mathutils.Matrix()
                
                def read_armature(arm_matrix,bone,parent = None):
                    if (parent and not bone.parent.name == parent.name):
                        return

                    matrix = mathutils.Matrix(bone.matrix)
                    
                    if parent:

                        #print("==== "+bone.name+" ====")
                        a = (bone.matrix_local)
                        
                        #print("A : [%.2f %.2f %.2f %.2f]" % (a[0][0], a[0][1], a[0][2], a[0][3]))
                        #print("    [%.2f %.2f %.2f %.2f]" % (a[1][0], a[1][1], a[1][2], a[1][3]))
                        #print("    [%.2f %.2f %.2f %.2f]" % (a[2][0], a[2][1], a[2][2], a[2][3]))
                        #print("    [%.2f %.2f %.2f %.2f]" % (a[3][0], a[3][1], a[3][2], a[3][3]))
                        
                        b = (parent.matrix_local.inverted().to_4x4())
                        
                        #print("B : [%.2f %.2f %.2f %.2f]" % (b[0][0], b[0][1], b[0][2], b[0][3]))
                        #print("    [%.2f %.2f %.2f %.2f]" % (b[1][0], b[1][1], b[1][2], b[1][3]))
                        #print("    [%.2f %.2f %.2f %.2f]" % (b[2][0], b[2][1], b[2][2], b[2][3]))
                        #print("    [%.2f %.2f %.2f %.2f]" % (b[3][0], b[3][1], b[3][2], b[3][3]))
                        
                        par_matrix = b * a
                        
                        transform = mathutils.Matrix([[1,0,0,0],[0,0,-1,0],[0,-1,0,0],[0,0,0,1]])
                        par_matrix = transform*par_matrix*transform
                        
                        # FIXME: that's ugly, find a clean way to change the matrix.....
                        if bpy.app.version[1] >= 62:
                            # blender 2.62 broke the API : Column-major access was changed to row-major access
                            # TODO: test me
                            par_matrix[1][3] = -par_matrix[1][3]
                            par_matrix[2][3] = -par_matrix[2][3]
                        else:
                            par_matrix[3][1] = -par_matrix[3][1]
                            par_matrix[3][2] = -par_matrix[3][2]
                        
                        #c = par_matrix
                        #print("With parent")
                        #print("C : [%.3f %.3f %.3f %.3f]" % (c[0][0], c[0][1], c[0][2], c[0][3]))
                        #print("    [%.3f %.3f %.3f %.3f]" % (c[1][0], c[1][1], c[1][2], c[1][3]))
                        #print("    [%.3f %.3f %.3f %.3f]" % (c[2][0], c[2][1], c[2][2], c[2][3]))
                        #print("    [%.3f %.3f %.3f %.3f]" % (c[3][0], c[3][1], c[3][2], c[3][3]))
                        
                    else:
                        
                        #print("==== "+bone.name+" ====")
                        #print("Without parent")

                        m = arm_matrix*bone.matrix_local
                        
                        #c = arm.matrix_world
                        #print("A : [%.3f %.3f %.3f %.3f]" % (c[0][0], c[0][1], c[0][2], c[0][3]))
                        #print("    [%.3f %.3f %.3f %.3f]" % (c[1][0], c[1][1], c[1][2], c[1][3]))
                        #print("    [%.3f %.3f %.3f %.3f]" % (c[2][0], c[2][1], c[2][2], c[2][3]))
                        #print("    [%.3f %.3f %.3f %.3f]" % (c[3][0], c[3][1], c[3][2], c[3][3]))
                        
                        #c = bone.matrix_local
                        #print("B : [%.3f %.3f %.3f %.3f]" % (c[0][0], c[0][1], c[0][2], c[0][3]))
                        #print("    [%.3f %.3f %.3f %.3f]" % (c[1][0], c[1][1], c[1][2], c[1][3]))
                        #print("    [%.3f %.3f %.3f %.3f]" % (c[2][0], c[2][1], c[2][2], c[2][3]))
                        #print("    [%.3f %.3f %.3f %.3f]" % (c[3][0], c[3][1], c[3][2], c[3][3]))
                        
                        par_matrix = m*mathutils.Matrix([[-1,0,0,0],[0,0,1,0],[0,1,0,0],[0,0,0,1]])
                                                
                        #c = par_matrix
                        #print("C : [%.3f %.3f %.3f %.3f]" % (c[0][0], c[0][1], c[0][2], c[0][3]))
                        #print("    [%.3f %.3f %.3f %.3f]" % (c[1][0], c[1][1], c[1][2], c[1][3]))
                        #print("    [%.3f %.3f %.3f %.3f]" % (c[2][0], c[2][1], c[2][2], c[2][3]))
                        #print("    [%.3f %.3f %.3f %.3f]" % (c[3][0], c[3][1], c[3][2], c[3][3]))
                        

                    bone_stack[bone.name] = [par_matrix,parent,bone]

                    if bone.children:
                        for child in bone.children: read_armature(arm_matrix,child,bone)

                for bone in arm.data.bones.values():
                    if not bone.parent:
                        read_armature(arm_matrix,bone)

                frame_count = first_frame
                
                last_frame = int(getArmatureAnimationEnd(arm))
                num_frames = last_frame - first_frame

                while frame_count <= last_frame:

                    the_scene.frame_set(int(frame_count), subframe=0.0)
                    
                    if DEBUG: print("        <frame id=", int(frame_count), ">")
                    arm_pose = arm.pose
                    arm_matrix = arm.matrix_world
                    
                    transform = mathutils.Matrix([[-1,0,0,0],[0,0,1,0],[0,1,0,0],[0,0,0,1]])
                    arm_matrix = transform*arm_matrix

                    for bone_name in arm.data.bones.keys():
                        #bone_matrix = mathutils.Matrix(arm_pose.bones[bone_name].poseMatrix)
                        bone_matrix = mathutils.Matrix(arm_pose.bones[bone_name].matrix)
                        
                        #print("(outer loop) bone_matrix for",bone_name,"=", bone_matrix)
                        
                        #print(bone_name,":",bone_matrix)
                        
                        #bone_matrix = bpy.data.scenes[0].objects[0].pose.bones['Bone'].matrix
                        
                        for ibone in bone_stack:
                            
                            bone = bone_stack[ibone]
                            
                            if bone[BONE_ITSELF].name == bone_name:
                                
                                if DEBUG: print("            <bone id=",ibone,"name=",bone_name,">")
                                
                                # == 2.4 exporter ==
                                #if bone_stack[ibone][1]:
                                #    par_matrix = Blender.Mathutils.Matrix(arm_pose.bones[bone_stack[ibone][1].name].poseMatrix)
                                #    bone_matrix *= par_matrix.invert()
                                #else:
                                #    if b3d_parameters.get("local-space"):
                                #        bone_matrix *= TRANS_MATRIX
                                #    else:
                                #        bone_matrix *= arm_matrix
                                #bone_loc = bone_matrix.translationPart()
                                #bone_rot = bone_matrix.rotationPart().toQuat()
                                #bone_rot.normalize()
                                #bone_sca = bone_matrix.scalePart()
                                #keys_stack.append([frame_count - first_frame.val+1,bone_name,bone_loc,bone_sca,bone_rot])
                                
                                # if has parent
                                if bone[BONE_PARENT]:
                                    par_matrix = mathutils.Matrix(arm_pose.bones[bone[BONE_PARENT].name].matrix)
                                    bone_matrix = par_matrix.inverted()*bone_matrix
                                else:
                                    if b3d_parameters.get("local-space"):
                                        bone_matrix = bone_matrix*mathutils.Matrix([[-1,0,0,0],[0,0,1,0],[0,1,0,0],[0,0,0,1]])
                                    else:
                                        
                                        #if frame_count == 1:
                                        #    print("====",bone_name,"====")
                                        #    print("arm_matrix = ", arm_matrix)
                                        #    print("bone_matrix = ", bone_matrix)
                                        
                                        bone_matrix = arm_matrix*bone_matrix
                                        
                                        #if frame_count == 1:
                                        #    print("arm_matrix*bone_matrix", bone_matrix)
                                        
                                
                                #print("bone_matrix =", bone_matrix)
                                
                                bone_sca = bone_matrix.to_scale()
                                bone_loc = bone_matrix.to_translation()
                                
                                # FIXME: silly tweaks to resemble the Blender 2.4 exporter output
                                if b3d_parameters.get("local-space"):
                                    
                                    bone_rot = bone_matrix.to_quaternion()
                                    bone_rot.normalize()
                                    
                                    
                                    if not bone[BONE_PARENT]:
                                        tmp = bone_rot.z
                                        bone_rot.z = bone_rot.y
                                        bone_rot.y = tmp
                                        
                                        bone_rot.x = -bone_rot.x
                                    else:
                                        tmp = bone_loc.z
                                        bone_loc.z = bone_loc.y
                                        bone_loc.y = tmp

                                else:
                                    bone_rot = bone_matrix.to_quaternion()
                                    bone_rot.normalize()

                                keys_stack.append([frame_count - first_frame+1, bone_name, bone_loc, bone_sca, bone_rot])
                                if DEBUG: print("                <loc>", bone_loc, "</loc>")
                                if DEBUG: print("                <rot>", bone_rot, "</rot>")
                                if DEBUG: print("                <scale>", bone_sca, "</scale>")
                                if DEBUG: print("            </bone>")

                    frame_count += 1

                    if DEBUG: print("        </frame>")

                #Blender.Set("curframe",0)
                #Blender.Window.Redraw()

            temp_buf.append(write_node_mesh(obj,obj_count,anim_data,exp_root)) #NODE MESH
            
            if anim_data:
                temp_buf.append(write_node_anim(num_frames)) #NODE ANIM

                for ibone in bone_stack:
                    if not bone_stack[ibone][BONE_PARENT]:
                        temp_buf.append(write_node_node(ibone)) #NODE NODE

            obj_count += 1

            if len(temp_buf) > 0:
                node_buf.append(write_chunk(b"NODE",b"".join(temp_buf)))
                temp_buf = []
            
            if DEBUG: print("    </mesh>")

        if b3d_parameters.get("cameras"):
            if obj.type == "CAMERA":
                data = obj.data
                matrix = obj.getMatrix("worldspace")
                matrix *= TRANS_MATRIX

                if data.type == "ORTHO":
                    cam_type = 2
                    cam_zoom = round(data.scale,4)
                else:
                    cam_type = 1
                    cam_zoom = round(data.lens,4)

                cam_near = round(data.clipStart,4)
                cam_far = round(data.clipEnd,4)

                node_name = ("CAMS"+"\n%s"%obj.name+"\n%s"%cam_type+\
                             "\n%s"%cam_zoom+"\n%s"%cam_near+"\n%s"%cam_far)
                temp_buf.append(write_string(node_name)) #Node Name

                position = matrix.translation_part()
                temp_buf.append(write_float_triplet(-position[0], position[1], position[2]))

                scale = matrix.scale_part()
                temp_buf.append(write_float_triplet(scale[0], scale[1], scale[2]))

                matrix *= mathutils.Matrix.Rotation(180,4,'Y')
                quat = matrix.to_quat()
                quat.normalize()

                temp_buf.append(write_float_quad(quat.w, quat.x, quat.y, -quat.z))

                if len(temp_buf) > 0:
                    node_buf.append(write_chunk(b"NODE",b"".join(temp_buf)))
                    temp_buf = []

        if b3d_parameters.get("lights"):
            if amb_light == 0:
                data = Blender.World.GetCurrent()

                amb_light = 1
                amb_color = (int(data.amb[2]*255) |(int(data.amb[1]*255) << 8) | (int(data.amb[0]*255) << 16))

                node_name = (b"AMBI"+"\n%s"%amb_color)
                temp_buf.append(write_string(node_name)) #Node Name

                temp_buf.append(write_float_triplet(0, 0, 0)) #Position X, Y, Z
                temp_buf.append(write_float_triplet(1, 1, 1)) #Scale X, Y, Z
                temp_buf.append(write_float_quad(1, 0, 0, 0)) #Rotation W, X, Y, Z

                if len(temp_buf) > 0:
                    node_buf.append(write_chunk(b"NODE",b"".join(temp_buf)))
                    temp_buf = []

            if obj.type == "LAMP":
                data = obj.getData()
                matrix = obj.getMatrix("worldspace")
                matrix *= TRANS_MATRIX

                if data.type == 0:
                    lig_type = 2
                elif data.type == 2:
                    lig_type = 3
                else:
                    lig_type = 1

                lig_angle = round(data.spotSize,4)
                lig_color = (int(data.b*255) |(int(data.g*255) << 8) | (int(data.r*255) << 16))
                lig_range = round(data.dist,4)

                node_name = ("LIGS"+"\n%s"%obj.name+"\n%s"%lig_type+\
                             "\n%s"%lig_angle+"\n%s"%lig_color+"\n%s"%lig_range)
                temp_buf.append(write_string(node_name)) #Node Name

                position = matrix.translation_part()
                temp_buf.append(write_float_triplet(-position[0], position[1], position[2]))
                if DEBUG: print("        <position>",-position[0],position[1],position[2],"</position>")

                scale = matrix.scale_part()
                temp_buf.append(write_float_triplet(scale[0], scale[1], scale[2]))
                
                if DEBUG: print("        <scale>",scale[0],scale[1],scale[2],"</scale>")

                matrix *= mathutils.Matrix.Rotation(180,4,'Y')
                quat = matrix.toQuat()
                quat.normalize()

                temp_buf.append(write_float_quad(quat.w, quat.x, quat.y, -quat.z))
                if DEBUG: print("        <rotation>", quat.w, quat.x, quat.y, quat.z, "</rotation>")

                if len(temp_buf) > 0:
                    node_buf.append(write_chunk(b"NODE","b".join(temp_buf)))
                    temp_buf = []
    
    if len(node_buf) > 0:
        if exp_root:
            main_buf += write_chunk(b"NODE",b"".join(root_buf) + b"".join(node_buf))
        else:
            main_buf += b"".join(node_buf)

        node_buf = []
        root_buf = []

    if DEBUG: print("</node>")

    return main_buf

# ==== Write NODE MESH Chunk ====
def write_node_mesh(obj,obj_count,arm_action,exp_root):
    global vertex_groups
    vertex_groups = []
    mesh_buf = bytearray()
    temp_buf = bytearray()

    if arm_action:
        data = obj.data
    else:
        data = obj.to_mesh(the_scene, True, 'PREVIEW')
    
    temp_buf += write_int(-1) #Brush ID
    temp_buf += write_node_mesh_vrts(obj, data, obj_count, arm_action, exp_root) #NODE MESH VRTS
    temp_buf += write_node_mesh_tris(obj, data, obj_count, arm_action, exp_root) #NODE MESH TRIS

    if len(temp_buf) > 0:
        mesh_buf += write_chunk(b"MESH",temp_buf)
        temp_buf = ""

    return mesh_buf

def build_vertex_groups(data):
    for f in getFaces(data):
        for v in f.vertices:
            vertex_groups.append({})


# ==== Write NODE MESH VRTS Chunk ====
def write_node_mesh_vrts(obj, data, obj_count, arm_action, exp_root):
    vrts_buf = bytearray()
    temp_buf = []
    obj_flags = 0
    
    #global time_in_a
    #global time_in_b
    #global time_in_b1
    #global time_in_b2
    #global time_in_b3
    #global time_in_b4

    #data = obj.getData(mesh = True)
    global the_scene
    
    # FIXME: port to 2.5 API?
    #orig_uvlayer = data.activeUVLayer

    if b3d_parameters.get("vertex-normals"):
        obj_flags += 1

    #if b3d_parameters.get("vertex-colors") and data.getColorLayerNames():
    if b3d_parameters.get("vertex-colors") and len(getVertexColors(data)) > 0:
        obj_flags += 2

    temp_buf.append(write_int(obj_flags)) #Flags
    #temp_buf += write_int(len(data.getUVLayerNames())) #UV Set
    temp_buf.append(write_int(len(getUVTextures(data)))) #UV Set
    temp_buf.append(write_int(2)) #UV Set Size

    # ---- Prepare the mesh "stack"
    build_vertex_groups(data)
    
    # ---- Fill the mesh "stack"
    if DEBUG: print("")
    if DEBUG: print("        <!-- Building vertex_groups -->\n")

    ivert = -1


    #if PROGRESS_VERBOSE:
    #    progress = 0
    #    print("    vertex_groups, face:",0,"/",len(getFaces(data)))
    
    the_scene.frame_set(1,subframe=0.0)
    
    if b3d_parameters.get("local-space"):
        mesh_matrix = mathutils.Matrix()
    else:
        mesh_matrix = obj.matrix_world.copy()
    
    #import time
        
    uv_layers_count = len(getUVTextures(data))
    for face in getFaces(data):
        
        if DEBUG: print("        <!-- Face",face.index,"-->")
        
        #if PROGRESS_VERBOSE:
        #    progress += 1
        #    if (progress % 50 == 0): print("    vertex_groups, face:",progress,"/",len(data.faces))
        
        per_face_vertices[face.index] = []
        
        for vertex_id,vert in enumerate(face.vertices):
            
            ivert += 1
                        
            per_face_vertices[face.index].append(ivert)
            
            #a = time.time()
                            
            if arm_action:
                v = mesh_matrix * data.vertices[vert].co
                vert_matrix = mathutils.Matrix.Translation(v)
            else:
                vert_matrix = mathutils.Matrix.Translation(data.vertices[vert].co)

            vert_matrix *= TRANS_MATRIX
            vcoord = vert_matrix.to_translation()

            temp_buf.append(write_float_triplet(vcoord.x, vcoord.z, vcoord.y))

            #b = time.time()
            #time_in_a += b - a
            
            if b3d_parameters.get("vertex-normals"):
                norm_matrix = mathutils.Matrix.Translation(data.vertices[vert].normal)

                if arm_action:
                    norm_matrix *= mesh_matrix

                norm_matrix *= TRANS_MATRIX
                normal_vector = norm_matrix.to_translation()
                
                temp_buf.append(write_float_triplet(normal_vector.x,  #NX
                                                    normal_vector.z,  #NY
                                                    normal_vector.y)) #NZ

            #c = time.time()
            #time_in_b += c - b

            if b3d_parameters.get("vertex-colors") and len(getVertexColors(data)) > 0:
                vertex_colors = getVertexColors(data)
                if vertex_id == 0:
                    vcolor = vertex_colors[0].data[face.index].color1
                elif vertex_id == 1:
                    vcolor = vertex_colors[0].data[face.index].color2
                elif vertex_id == 2:
                    vcolor = vertex_colors[0].data[face.index].color3
                elif vertex_id == 3:
                    vcolor = vertex_colors[0].data[face.index].color4
                
                temp_buf.append(write_float_quad(vcolor.r, #R
                                                 vcolor.g, #G
                                                 vcolor.b, #B
                                                 1.0))     #A (FIXME?)
            
            #d = time.time()
            #time_in_b1 += d - c
            
            for vg in obj.vertex_groups:
                w = 0.0
                try:
                    w = vg.weight(vert)
                except:
                    pass
                vertex_groups[ivert][vg.name] = w
            
            #e = time.time()
            #time_in_b2 += e - d
            
            # ==== !!bottleneck here!! (40% of the function)
            if vertex_id == 0:
                for iuvlayer in range(uv_layers_count):
                    uv = getUVTextures(data)[iuvlayer].data[face.index].uv1
                    temp_buf.append(write_float_couple(uv[0], 1-uv[1]) ) # U, V
            elif vertex_id == 1:
                for iuvlayer in range(uv_layers_count):
                    uv = getUVTextures(data)[iuvlayer].data[face.index].uv2
                    temp_buf.append(write_float_couple(uv[0], 1-uv[1]) ) # U, V
            elif vertex_id == 2:
                for iuvlayer in range(uv_layers_count):
                    uv = getUVTextures(data)[iuvlayer].data[face.index].uv3
                    temp_buf.append(write_float_couple(uv[0], 1-uv[1]) ) # U, V
            elif vertex_id == 3:
                for iuvlayer in range(uv_layers_count):
                    uv = getUVTextures(data)[iuvlayer].data[face.index].uv4
                    temp_buf.append(write_float_couple(uv[0], 1-uv[1]) ) # U, V

            #f = time.time()
            #time_in_b3 += f - e
            # =====================

    
    if DEBUG: print("")
    
    #c = time.time()
    #time_in_b += c - b
    #print("time_in_a = ",time_in_a)
    #print("time_in_b = ",time_in_b)
    #print("time_in_b1 = ", time_in_b1)
    #print("time_in_b2 = ", time_in_b2)
    #print("time_in_b3 = ", time_in_b3)
    #print("time_in_b4 = ", time_in_b4)

    if len(temp_buf) > 0:
        vrts_buf += write_chunk(b"VRTS",b"".join(temp_buf))
        temp_buf = []

    return vrts_buf

# ==== Write NODE MESH TRIS Chunk ====
def write_node_mesh_tris(obj, data, obj_count,arm_action,exp_root):

    global texture_count

    #FIXME?
    #orig_uvlayer = data.activeUVLayer

    # An dictoriary that maps all brush-ids to a list of faces
    # using this brush. This helps to sort the triangles by
    # brush, creating less mesh buffer in irrlicht.
    dBrushId2Face = {}
    
    if DEBUG: print("")
    
    for face in getFaces(data):
        img_found = 0
        face_stack = []
        
        uv_textures = getUVTextures(data)
        uv_layer_count = len(uv_textures)
        for iuvlayer,uvlayer in enumerate(uv_textures):
            if iuvlayer < 8:
                
                if iuvlayer >= uv_layer_count:
                    continue
                
                img_id = -1
                
                img = uv_textures[iuvlayer].data[face.index].image
                if img:
                    
                    if img.filepath in trimmed_paths:
                        img_name = trimmed_paths[img.filepath]
                    else:
                        img_name = os.path.basename(img.filepath)
                        trimmed_paths[img.filepath] = img_name
                    
                    img_found = 1
                    if img_name in texs_stack:
                        img_id = texs_stack[img_name][TEXTURE_ID]

                face_stack.insert(iuvlayer,img_id)

        for i in range(len(face_stack),texture_count):
            face_stack.append(-1)

        if img_found == 0:
            brus_id = -1
            if data.materials and data.materials[face.material_index]:
                mat_name = data.materials[face.material_index].name
                for i in range(len(brus_stack)):
                    if brus_stack[i] == mat_name:
                        brus_id = i
                        break
            else:
                for i in range(len(brus_stack)):
                    if brus_stack[i] == face_stack:
                        brus_id = i
                        break
        else:
            brus_id = -1
            for i in range(len(brus_stack)):
                if brus_stack[i] == face_stack:
                    brus_id = i
                    break
            if brus_id == -1:
                print("Cannot find in brus stack : ", face_stack)

        if brus_id in dBrushId2Face:
            dBrushId2Face[brus_id].append(face)
        else:
            dBrushId2Face[brus_id] = [face]
        
        if DEBUG: print("        <!-- Face",face.index,"in brush",brus_id,"-->")
    
    tris_buf = bytearray()
    
    if DEBUG: print("")
    if DEBUG: print("        <!-- TRIS chunk -->")
    
    if PROGRESS_VERBOSE: progress = 0
                
    for brus_id in dBrushId2Face.keys():
        
        if PROGRESS_VERBOSE:
            progress += 1
            print("BRUS:",progress,"/",len(dBrushId2Face.keys()))
        
        temp_buf = [write_int(brus_id)] #Brush ID
        
        if DEBUG: print("        <brush id=", brus_id, ">")
        
        if PROGRESS_VERBOSE: progress2 = 0
                
        for face in dBrushId2Face[brus_id]:
            
            if PROGRESS_VERBOSE:
                progress2 += 1
                if (progress2 % 50 == 0): print("    TRIS:",progress2,"/",len(dBrushId2Face[brus_id]))

            vertices = per_face_vertices[face.index]

            temp_buf.append(write_int(vertices[2])) #A
            temp_buf.append(write_int(vertices[1])) #B
            temp_buf.append(write_int(vertices[0])) #C

            if DEBUG: print("            <face id=", vertices[2], vertices[1], vertices[0],"/> <!-- face",face.index,"-->")

            if len(face.vertices) == 4:
                temp_buf.append(write_int(vertices[3])) #A
                temp_buf.append(write_int(vertices[2])) #B
                temp_buf.append(write_int(vertices[0])) #C
                if DEBUG: print("            <face id=", vertices[3], vertices[2], vertices[0],"/> <!-- face",face.index,"-->")

        if DEBUG: print("        </brush>")
        tris_buf += write_chunk(b"TRIS", b"".join(temp_buf))

    return tris_buf

# ==== Write NODE ANIM Chunk ====
def write_node_anim(num_frames):
    anim_buf = bytearray()
    temp_buf = bytearray()

    temp_buf += write_int(0) #Flags
    temp_buf += write_int(num_frames) #Frames
    temp_buf += write_float(60) #FPS

    if len(temp_buf) > 0:
        anim_buf += write_chunk(b"ANIM",temp_buf)
        temp_buf = ""

    return anim_buf

# ==== Write NODE NODE Chunk ====
def write_node_node(ibone):
    node_buf = bytearray()
    temp_buf = []

    bone = bone_stack[ibone]

    matrix = bone[BONE_PARENT_MATRIX]
    temp_buf.append(write_string(bone[BONE_ITSELF].name)) #Node Name
    


    # FIXME: we should use the same matrix format everywhere to not require this
    
    position = matrix.to_translation()
    if bone[BONE_PARENT]:
        temp_buf.append(write_float_triplet(-position[0], position[2], position[1]))
    else:
        temp_buf.append(write_float_triplet(position[0], position[2], position[1]))
    
    
    scale = matrix.to_scale()
    temp_buf.append(write_float_triplet(scale[0], scale[2], scale[1]))

    quat = matrix.to_quaternion()
    quat.normalize()

    temp_buf.append(write_float_quad(quat.w, quat.x, quat.z, quat.y))

    temp_buf.append(write_node_bone(ibone))
    temp_buf.append(write_node_keys(ibone))

    for iibone in bone_stack:
        if bone_stack[iibone][BONE_PARENT] == bone_stack[ibone][BONE_ITSELF]:
            temp_buf.append(write_node_node(iibone))

    if len(temp_buf) > 0:
        node_buf += write_chunk(b"NODE", b"".join(temp_buf))
        temp_buf = []

    return node_buf

# ==== Write NODE BONE Chunk ====
def write_node_bone(ibone):
    bone_buf = bytearray()
    temp_buf = []

    my_name = bone_stack[ibone][BONE_ITSELF].name

    for ivert in range(len(vertex_groups)):
        if my_name in vertex_groups[ivert]:
            vert_influ = vertex_groups[ivert][my_name]
            #if DEBUG: print("        <bone name=",bone_stack[ibone][BONE_ITSELF].name,"face_vertex_id=", ivert + iuv,
            #                " weigth=", vert_influ[1] , "/>")
            temp_buf.append(write_int(ivert)) # Face Vertex ID
            temp_buf.append(write_float(vert_influ)) #Weight

    bone_buf += write_chunk(b"BONE", b"".join(temp_buf))
    temp_buf = []

    return bone_buf

# ==== Write NODE KEYS Chunk ====
def write_node_keys(ibone):
    keys_buf = bytearray()
    temp_buf = []

    temp_buf.append(write_int(7)) #Flags

    my_name = bone_stack[ibone][BONE_ITSELF].name

    for ikeys in range(len(keys_stack)):
        if keys_stack[ikeys][1] == my_name:
            temp_buf.append(write_int(keys_stack[ikeys][0])) #Frame

            position = keys_stack[ikeys][2]
            # FIXME: we should use the same matrix format everywhere and not require this
            if b3d_parameters.get("local-space"):
                if bone_stack[ibone][BONE_PARENT]:
                    temp_buf.append(write_float_triplet(-position[0], position[2], position[1]))
                else:
                    temp_buf.append(write_float_triplet(position[0], position[2], position[1]))
            else:
                temp_buf.append(write_float_triplet(-position[0], position[1], position[2]))

            scale = keys_stack[ikeys][3]
            temp_buf.append(write_float_triplet(scale[0], scale[1], scale[2]))

            quat = keys_stack[ikeys][4]
            quat.normalize()

            temp_buf.append(write_float_quad(quat.w, -quat.x, quat.y, quat.z))
            #break

    keys_buf += write_chunk(b"KEYS",b"".join(temp_buf))
    temp_buf = []

    return keys_buf


# ==== CONFIRM OPERATOR ====
class B3D_Confirm_Operator(bpy.types.Operator):
    bl_idname = ("screen.b3d_confirm")
    bl_label = ("File Exists, Overwrite?")
    
    def invoke(self, context, event):
        wm = context.window_manager
        return wm.invoke_props_dialog(self)
    
    def execute(self, context):
        write_b3d_file(B3D_Confirm_Operator.filepath)
        return {'FINISHED'}


#class ObjectListItem(bpy.types.PropertyGroup):
#    id = bpy.props.IntProperty(name="ID")
#
#bpy.utils.register_class(ObjectListItem)
    
# ==== EXPORT OPERATOR ====

class B3D_Export_Operator(bpy.types.Operator):
    bl_idname = ("screen.b3d_export")
    bl_label = ("B3D Export")
    filepath = bpy.props.StringProperty(subtype="FILE_PATH")

    selected = bpy.props.BoolProperty(name="Export Selected Only", default=False)
    vnormals = bpy.props.BoolProperty(name="Export Vertex Normals", default=True)
    vcolors  = bpy.props.BoolProperty(name="Export Vertex Colors", default=False)
    cameras  = bpy.props.BoolProperty(name="Export Cameras", default=False)
    lights   = bpy.props.BoolProperty(name="Export Lights", default=False)
    mipmap   = bpy.props.BoolProperty(name="Mipmap", default=False)
    localsp  = bpy.props.BoolProperty(name="Use Local Space Coords", default=False)
    textures = bpy.props.BoolProperty(name="Export links to texture files", default=False)

    overwrite_without_asking  = bpy.props.BoolProperty(name="Overwrite without asking", default=False)
    
    #skip_dialog = False
    
    #objects = bpy.props.CollectionProperty(type=ObjectListItem, options={'HIDDEN'})
    
    def invoke(self, context, event):
        blend_filepath = context.blend_data.filepath
        if not blend_filepath:
            blend_filepath = "Untitled.b3d"
        else:
            blend_filepath = os.path.splitext(blend_filepath)[0] + ".b3d"
        self.filepath = blend_filepath
        
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}
    
    def execute(self, context):
        
        global b3d_parameters
        global the_scene
        b3d_parameters["export-selected"] = self.selected
        b3d_parameters["vertex-normals" ] = self.vnormals
        b3d_parameters["vertex-colors"  ] = self.vcolors
        b3d_parameters["cameras"        ] = self.cameras
        b3d_parameters["lights"         ] = self.lights
        b3d_parameters["mipmap"         ] = self.mipmap
        b3d_parameters["local-space"    ] = self.localsp
        b3d_parameters["export-textures"] = self.textures
        
        the_scene = context.scene
        
        if self.filepath == "":
            return {'FINISHED'}

        if not self.filepath.endswith(".b3d"):
            self.filepath += ".b3d"

        print("EXPORT", self.filepath," vcolor = ", self.vcolors)
            
        obj_list = []
        try:
            # FIXME: silly and ugly hack, the list of objects to export is passed through
            #        a custom scene property
            obj_list = context.scene.obj_list
        except:
            pass
        
        if len(obj_list) > 0:
          
            #objlist = []
            #for a in self.objects:
            #    objlist.append(bpy.data.objects[a.id])
            #
            #write_b3d_file(self.filepath, obj_list)
            
            write_b3d_file(self.filepath, obj_list)
        else:
            if os.path.exists(self.filepath) and not self.overwrite_without_asking:
                #self.report({'ERROR'}, "File Exists")
                B3D_Confirm_Operator.filepath = self.filepath
                bpy.ops.screen.b3d_confirm('INVOKE_DEFAULT')
                return {'FINISHED'}
            else:
                write_b3d_file(self.filepath)
        return {'FINISHED'}


# Add to a menu
def menu_func_export(self, context):
    global the_scene
    the_scene = context.scene
    self.layout.operator(B3D_Export_Operator.bl_idname, text="B3D (.b3d)")

def register():
    bpy.types.INFO_MT_file_export.append(menu_func_export)
    bpy.utils.register_module(__name__)

def unregister():
    bpy.types.INFO_MT_file_export.remove(menu_func_export)

if __name__ == "__main__":
    register()
    
