from pathlib import Path
import os

def remove_duplicates(values):
    indices = {}
    filtered_values = []
    for v in values:
        if v in indices:
            continue
        i = len(filtered_values)
        filtered_values.append(v)
        indices[v] = i

    index_map = {
        i: indices[v]
        for i, v in enumerate(values)
    }
    return filtered_values, index_map
            

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
            pos = tuple([pos[0], pos[2], pos[1]])
            positions.append(pos)
        elif kind == 'vt':
            assert len(args) == 2, 'uv must have 2 coordinates.'
            uv = [float(x) for x in args]
            uv[0] = 1.0 - uv[0]
            uv[1] = 1.0 - uv[1]
            uv = tuple(32.0*x for x in uv)
            uvs.append(uv)
        elif kind == 'vn':
            assert len(args) == 3, 'normal must have 3 coordinates.'
            norm = ([float(x) for x in args])
            # swap y/z
            norm = tuple([norm[0], norm[2], norm[1]])
            normals.append(norm)
        elif kind == 'f':
            assert len(args) == 3, 'face must be a triangle.'
            face = []
            for arg in args:
                indices = arg.split('/')
                assert len(indices) == 3, 'face must have 3 indices per vertex - position, uv, normal.'
                a,b,c = [int(x)-1 for x in indices]
                face.append([a, b, c])
            faces.append(face)

    positions, position_index_map = remove_duplicates(positions)
    uvs, uv_index_map = remove_duplicates(uvs)
    normals, norm_index_map = remove_duplicates(normals)

    faces = [
        tuple(
            x
            for (pos_idx, uv_idx, norm_idx) in face
            for x in [
                # convert indices to offsets.
                3*position_index_map[pos_idx],
                2*uv_index_map[uv_idx],
                3*norm_index_map[norm_idx],
            ]
        )
        for face in faces
    ]

    faces = sorted(set(faces))

    return positions, uvs, normals, faces


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
                    positions, texcoords, normals, tris = load_object(file)
                    float_arrays = [
                        (positions, 'positions'),
                        (texcoords, 'texcoords'),
                        (normals, 'normals'),
                    ]
                    h_file.write(
                        f'extern model_t {model_name}_model;\n'
                    )
                    for array, array_name in float_arrays:
                        c_file.write(
                            f'static float {model_name}_{array_name}[] = ' + '{\n'
                                + '\n'.join(
                                    '\t' + ', '.join(map(str, x)) + ','
                                    for x in array
                                )
                                + '\n};\n'
                        )
                    c_file.write(
                        f'static uint16_t {model_name}_triangles[] = ' + '{\n'
                            + '\n'.join(
                                '\t' + ', '.join(str(i) for i in tri) + ','
                                for tri in tris
                            )
                            + '\n};\n'
                    )
                    c_file.write(
                        f'model_t {model_name}_model = MODEL(\n'
                        + f'\t{model_name}_positions,\n'
                        + f'\t{model_name}_texcoords,\n'
                        + f'\t{model_name}_normals,\n'
                        + f'\t{model_name}_triangles);\n'
                    )
                    

if __name__ == '__main__':
	main()
