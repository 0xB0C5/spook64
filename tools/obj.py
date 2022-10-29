from pathlib import Path
import os

def remove_unnecessary(values, used):
    indices = {}
    filtered_values = []
    index_map = {}
    for og_index, v in enumerate(values):
        if og_index not in used:
            continue
        if v in indices:
            index_map[og_index] = indices[v]
            continue
        i = len(filtered_values)
        filtered_values.append(v)
        indices[v] = i
        index_map[og_index] = i

    return filtered_values, index_map

def get_connected_vertices(v0, faces):
    visited = set([v0])

    def _get_connected_vertices(v0):
        for face in faces:
            a = face[0][0]
            b = face[1][0]
            c = face[2][0]
            if v0 not in [a,b,c]:
                continue
            for v1 in [a,b,c]:
                if v1 in visited:
                    continue
                visited.add(v1)
                _get_connected_vertices(v1)

    _get_connected_vertices(v0)
    return visited 


def minimize_model(positions, uvs, normals, faces):
    
    used_positions = set(a for face in faces for (a,b,c) in face)
    used_uvs = set(b for face in faces for (a,b,c) in face)
    used_normals = set(c for face in faces for (a,b,c) in face)

    positions, position_index_map = remove_unnecessary(positions, used_positions)
    uvs, uv_index_map = remove_unnecessary(uvs, used_uvs)
    normals, norm_index_map = remove_unnecessary(normals, used_normals)

    faces = [
        tuple(
            (
                position_index_map[pos_idx],
                uv_index_map[uv_idx],
                norm_index_map[norm_idx],
            )
            for (pos_idx, uv_idx, norm_idx) in face
        )
        for face in faces
    ]

    faces = sorted(set(faces))

    return positions, uvs, normals, faces


def load_objects_raw(file, is_snooper):
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
                face.append((a, b, c))
            faces.append(tuple(face))

    positions, uvs, normals, faces = minimize_model(positions, uvs, normals, faces)

    if not is_snooper:
        yield positions, uvs, normals, faces
        return

    # uhhh for whatever reason the snooper is too big
    # idk how to use blender
    # just scale the positions
    positions = [tuple(0.6*x for x in pos) for pos in positions]

    feet = set()
    head = set()
    remaining = set(range(len(positions)))

    while len(remaining) > 0:
        component = get_connected_vertices(next(iter(remaining)), faces)
        remaining -= component
        if len(component) <= 4:
            feet |= component
        else:
            head |= component

    yield sub_model(feet, positions, uvs, normals, faces)
    yield sub_model(head, positions, uvs, normals, faces)

def sub_model(pos_indices, positions, uvs, normals, faces):
    faces = [
        face for face in faces
        if all(a in pos_indices for (a,b,c) in face)
    ]
    return minimize_model(positions, uvs, normals, faces)

    
def load_objects(file, is_snooper):
    for positions, uvs, normals, faces in load_objects_raw(file, is_snooper):
        faces = [
            tuple(
                x
                for (pos_idx, uv_idx, norm_idx) in face
                for x in [
                    # convert indices to offsets.
                    3*pos_idx,
                    2*uv_idx,
                    3*norm_idx,
                ]
            )
            for face in faces
        ]
        yield positions, uvs, normals, faces



def main():
    root_dir = Path(__file__).parent.parent
    blender_dir = root_dir / 'blender'

    with open(root_dir / 'src' / '_generated_models.c', 'w') as c_file:
        with open(root_dir / 'src' / '_generated_models.h', 'w') as h_file:
            c_file.write('#include "render.h"\n')

            for filename in os.listdir(blender_dir):
                if not filename.endswith('.obj'):
                    continue
                # I didn't split the feet of the snooper in blender
                # so I'm splitting it here instead.
                is_snooper = filename.startswith('snooper')
                with open(blender_dir / filename) as file:
                    for (positions, texcoords, normals, tris) in load_objects(file, is_snooper):
                        model_name = filename.split('.')[0]
                        if is_snooper and len(positions) <= 8:
                            model_name += '_feet'
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
