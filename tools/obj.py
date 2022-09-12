from pathlib import Path
import os

def load_object(file):
    positions = []
    uvs = []
    normals = []
    faces = []
    for line in file.readlines():
        line = line.strip()
        if not line:
            continue
        parts = line.split(' ')
        kind = parts[0]
        args = parts[1:]
        if kind == 'v':
            assert len(args) == 3, 'vertex position must have 3 coordinates.'
            pos = ([float(x) for x in args])
            # swap y/z
            pos = [pos[0], pos[2], pos[1]]
            positions.append(pos)
        elif kind == 'vt':
            assert len(args) == 2, 'uv must have 2 coordinates.'
            uv = [float(x) for x in args]
            uv[0] = 1.0 - uv[0]
            uv[1] = 1.0 - uv[1]
            uv = [32.0*x for x in uv]
            uvs.append(uv)
        elif kind == 'vn':
            assert len(args) == 3, 'normal must have 3 coordinates.'
            norm = ([float(x) for x in args])
            # swap y/z
            norm = [norm[0], norm[2], norm[1]]
            normals.append(norm)
        elif kind == 'f':
            assert len(args) == 3, 'face must be a triangle.'
            face = []
            for arg in args:
                indices = arg.split('/')
                assert len(indices) == 3, 'face must have 3 indices per vertex - position, uv, normal.'
                face.append([int(x)-1 for x in indices])
            faces.append(face)

    full_vertices = []
    full_vertex_indices = {}
    full_faces = []

    for face in faces:
        full_face = []
        for pos_idx, uv_idx, norm_idx in face:
            vertex = positions[pos_idx] + uvs[uv_idx] + normals[norm_idx]
            vertex = tuple(vertex)
            vert_idx = full_vertex_indices.get(vertex)
            if vert_idx is None:
                vert_idx = len(full_vertices)
                full_vertex_indices[vertex] = vert_idx
                full_vertices.append(vertex)
            full_face.append(vert_idx)
        full_faces.append(full_face)

    return full_vertices, full_faces

def main():
    root_dir = Path(__file__).parent.parent
    blender_dir = root_dir / 'blender'
    
    with open(root_dir / 'src' / '_generated_models.c', 'w') as c_file:
        with open(root_dir / 'src' / '_generated_models.h', 'w') as h_file:
            c_file.write('#include "render.h"\n')

            for filename in os.listdir(blender_dir):
                model_name = filename.split('.')[0]
                if not filename.endswith('.obj'):
                    continue
                with open(blender_dir / filename) as file:
                    verts, tris = load_object(file)
                    h_file.write(
                        f'extern model_t {model_name}_model;\n'
                    )
                    c_file.write(
                        f'static float {model_name}_vertices[] = ' + '{\n'
                            + '\n'.join(
                                '\t' + ', '.join(map(str, vert)) + ','
                                for vert in verts
                            )
                            + '\n};\n'
                    )
                    c_file.write(
                        f'static uint16_t {model_name}_triangles[] = ' + '{\n'
                            + '\n'.join(
                                '\t' + ', '.join(map(str, tri)) + ','
                                for tri in tris
                            )
                            + '\n};\n'
                    )
                    c_file.write(
                        f'model_t {model_name}_model = MODEL({model_name}_vertices, {model_name}_triangles);'
                    )
                    

if __name__ == '__main__':
	main()
