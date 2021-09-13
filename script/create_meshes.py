import io
import struct
import sys

from pathlib import PurePath

for file_name in sys.argv[1:]:
    vertex_file_name = PurePath(file_name)
    vertex_file_name = vertex_file_name.with_suffix('.vertex')

    with open(file_name, mode="rb") as stl, open(vertex_file_name, mode="wb") as vertex:
        header = stl.read(80)
        num_triangles_b = stl.read(4)
        num_triangles = int.from_bytes(num_triangles_b, byteorder='little')
        num_vertices_b = struct.pack('I', num_triangles * 3)

        vertex.write(num_vertices_b)

        for t in range(num_triangles):
            normal_vector = struct.unpack('fff', stl.read(12))
            vertex1_b = stl.read(12)
            vertex1 = struct.unpack('fff', vertex1_b)

            vertex2_b = stl.read(12)
            vertex2 = struct.unpack('fff', vertex2_b)

            vertex3_b = stl.read(12)
            vertex3 = struct.unpack('fff', vertex3_b)

            centroid_x = (vertex1[0] + vertex2[0] + vertex3[0]) / 3
            centroid_y = (vertex1[1] + vertex2[1] + vertex3[1]) / 3
            centroid_z = (vertex1[2] + vertex2[2] + vertex3[2]) / 3
            centroid_b = struct.pack('fff', centroid_x, centroid_y, centroid_z)

            vertex.write(vertex1_b)
            vertex.write(centroid_b)
            vertex.write(vertex2_b)
            vertex.write(centroid_b)
            vertex.write(vertex3_b)
            vertex.write(centroid_b)

            num_attr = int.from_bytes(stl.read(2), byteorder='little')
            stl.read(num_attr)
