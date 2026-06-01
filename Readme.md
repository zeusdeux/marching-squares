# Naive marching squares

Playing around with a basic contouring algorithm by attempting to
contour simplex3D noise.

Marching squares &mdash; https://en.wikipedia.org/wiki/Marching_squares.

A gif should load below:

![example](./example.gif)

On macos:

```sh
make run
```

> This has only been built and run on macos. I haven't tested on
> windows nor linux but should be straightforward given we are using
> raylib to handle the windowing and graphics. Also, I have done the
> simplest of things just to understand how contouring works using
> marching squares. It renders smooth as butter at 120fps.

## Explanation

### First we need a contouring threshold aka `isovalue`

An **isovalue** (also called a threshold) is the value we are contouring at.

In this codebase, it's `ISOVALUE` and defaults to `0.5`:

```text
isovalue = 0.5
```

Any corner:

```text
value > 0.5  => ON
value <= 0.5 => OFF
```

Let's take an example with the following corner values:

```text
A (0 ,  0) = 1.0   ON
B (80,  0) = 0.2   OFF
C (80, 80) = 0.3   OFF
D (0 , 80) = 0.4   OFF
```

So only A is ON.

#### Example square

```text
A ---- B
|      |
|      |
D ---- C
```

Coordinates:

```text
A = (0 ,  0)   value=1.0
B = (80,  0)   value=0.2
C = (80, 80)   value=0.3
D = (0 , 80)   value=0.4
```

Values can also be thought of as weights. In other words, we are just
describing a field of scalar values where the field has points and
points have a scalar value or weight.

### Without interpolation

A simple marching squares implementation placing intersections at edge midpoints:

```text
A----X----B
|         |
X         |
|         |
D---------C
```

Endpoints:

```text
(40,  0)
(0 , 40)
```

Line:

```text
(40, 0) -> (0, 40)
```

While, this works, _it looks blocky_.

### With interpolation (lerp)

Instead of using the midpoint, lets find where the scalar field
crosses the isovalue aka where does our example isovalue of `0.5` lie
between the value of/weight assigned to each point that make up an edge (e.g., A->B).

```text
lerp(A, B, t) = A + t*(B - A)
```

#### Edge A→B

Values:

```text
A   = 1.0
B   = 0.2
iso = 0.5
```

Compute interpolation parameter:

```text
t = (iso - valueA) / (valueB - valueA)
```

Substitute:

```text
t = (0.5 - 1.0) / (0.2 - 1.0)
  = -0.5 / -0.8
  = 0.625
```

So the contour crosses **62.5%** of the way from A toward B.


```text
A==================X==========B
                   |
0.0             0.625       1.0
```

Remember that `lerp` is

```text
lerp(A, B, t) = A + t*(B - A)
```

Position:

```text
P = lerp(A, B, t)
x = 0 + (80 - 0)*0.625 => 50
y = 0 + (0  - 0)*0.625 => 0
```

Intersection:

```text
P = (50, 0)
```

#### Edge D→A

Values:

```text
D   = 0.4
A   = 1.0
iso = 0.5
```

Compute:

```text
t = (0.5 - 0.4) / (1.0 - 0.4)
  = 0.1 / 0.6
  = 0.1666667
```

```text
D===X=========================A
    |
0.0 0.1667                  1.0
```

Position:

```text
P = lerp(D, A, 0.1667)

x = 0
y = 80 + (0-80)*0.1667
  = 66.67
```

Intersection:

```text
P = (0, 66.67)
```

#### Final isoline

```text
A(1.0) -------X(50, 0)------ B(0.2)
 |                           |
 |                           |
 |                           |
X(0, 66.67)                  |
 |                           |
D(0.4) --------------------- C(0.3)
```

We draw:

```text
(50, 0) -> (0, 66.67)
```

### Why this formula works

Suppose values on an edge change linearly:

```text
1.0 ------------------- 0.2
A                       B
```

You want the point where value becomes:

```text
0.5
```

The fraction along the edge is:

```text
(valueWanted - startValue)
--------------------------
(endValue    - startValue)
```

which gives:

```text
t = (iso - v1) / (v2 - v1)
```

Then we use that same fraction to interpolate position:

```text
position = lerp(p1, p2, t)
```

This is how marching squares uses lerp &mdash; first interpolate
**scalar values** to find **t**, then interpolate **positions** using
the same **t**.
