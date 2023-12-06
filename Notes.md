# STAGES
1. Parse SVG file, or just get some inputs
2. Encode all the paths into some structure, that will be then sent on the GPU
3. Parallel transform each control point in each curve in each path by the corresponding path transform matrix
- Can be done parallel for each of the curve - dispatch as many workgroups as number of paths, then in each workgroup, dispatch 256 threads
  and transform 1 curve by each thread (1 thread may need to transform up to 3 control points)
4. Determine number of segments for each curve. Paths are now transformed - each control point is transformed
- Can be done parallel for each curve again - dispatch as many workgroups as number of paths, in each workgroup, dispatch 256 threads
  that will work seperatly on each of the curve
5. Flatten each curve, where we also compute corresponding bounding box for each path
- Dispatch as many workgroups as there are paths - in each WG, dispatch 256 threads, reading curves until all curves are read, and flatten each of the curve
- in one iteration, before loading another 256 curves
6. To be continued...