// Written by tools/render_faceplate.ts (mise run faceplate): don't edit. Preview (F5) keeps the colors.
$fn = 64;
union()
{
  color(c = "gold", alpha = 1)
  {
    union()
    {
      difference()
      {
        union()
        {
          translate(v = [0, 0, 0])
          {
            linear_extrude(height = 14.4, center = false, convexity = undef, twist = undef, slices = undef, scale = 1, $fn = 20)
            {
              offset(r = 3, delta = undef, chamfer = undef)
              {
                square(size = [55.800000000000004, 35.300000000000004], center = true);
              }
            }
          }
          hull()
          {
            translate(v = [-27.900000000000002, -26.32793565455276, 0])
            {
              cube(size = [55.800000000000004, 5.68793565455276, 0.01], center = false);
            }
            translate(v = [-27.900000000000002, -21.086764281119446, 14.39])
            {
              cube(size = [55.800000000000004, 0.4467642811194459, 0.009999999999999787], center = false);
            }
          }
        }
        translate(v = [-29.3, -19.05, 5.4])
        {
          cube(size = [58.6, 38.1, 10], center = false);
        }
        translate(v = [29, 0, 0])
        {
          rotate(a = [0, 0, -90], v = undef)
          {
            union()
            {
              union()
              {
                translate(v = [-16, -49, -1])
                {
                  cube(size = [32, 42.5, 2.21], center = false);
                }
                hull()
                {
                  translate(v = [-16.8, -49.8, -1])
                  {
                    cube(size = [33.6, 44.099999999999994, 1], center = false);
                  }
                  translate(v = [-16, -49, 0.8])
                  {
                    cube(size = [32, 42.5, 0.010000000000000009], center = false);
                  }
                }
              }
              union()
              {
                translate(v = [-17.900000000000002, -53.8, 1.2])
                {
                  cube(size = [35.800000000000004, 49.599999999999994, 5.2], center = false);
                }
                translate(v = [-19.05, -58.3, 4.2])
                {
                  cube(size = [38.1, 58.599999999999994, 2.2], center = false);
                }
                translate(v = [-10.3, -58.3, 2.7])
                {
                  cube(size = [20.6, 4.509999999999998, 3.7], center = false);
                }
                translate(v = [-10.3, -4.21, 2.7])
                {
                  cube(size = [20.6, 4.51, 3.7], center = false);
                }
              }
            }
          }
        }
        translate(v = [-31.900000000000002, -6, 5.6])
        {
          cube(size = [2.610000000000003, 12, 9.8], center = false);
        }
        translate(v = [-4.3, -27.32793565455276, 7.1000000000000005])
        {
          cube(size = [8.6, 8.287935654552761, 1.6499999999999995], center = false);
        }
        translate(v = [-4.3, 19.04, 7.1000000000000005])
        {
          cube(size = [8.6, 8.287935654552761, 1.6499999999999995], center = false);
        }
      }
      union()
      {
        translate(v = [-26.8, -16.55, 4.19])
        {
          cylinder(h = 2.71, r = 0.95, center = false);
        }
        translate(v = [-26.8, 16.55, 4.19])
        {
          cylinder(h = 2.71, r = 0.95, center = false);
        }
        translate(v = [26.8, -16.55, 4.19])
        {
          cylinder(h = 2.71, r = 0.95, center = false);
        }
        translate(v = [26.8, 16.55, 4.19])
        {
          cylinder(h = 2.71, r = 0.95, center = false);
        }
      }
    }
  }
  intersection()
  {
    translate(v = [29, 0, 0])
    {
      rotate(a = [0, 0, -90], v = undef)
      {
        union()
        {
          color(c = "black", alpha = 1)
          {
            translate(v = [-17.6, -53.5, 1.2])
            {
              cube(size = [35.2, 48.5, 3], center = false);
            }
          }
          color(c = "orangered", alpha = 1)
          {
            translate(v = [-15.5, -48.5, 1.15])
            {
              cube(size = [31, 41.5, 0.050000000000000044], center = false);
            }
          }
          color(c = "royalblue", alpha = 1)
          {
            translate(v = [-18.75, -58, 4.2])
            {
              cube(size = [37.5, 58, 1.2000000000000002], center = false);
            }
          }
          color(c = "goldenrod", alpha = 1)
          {
            translate(v = [-10, -3, 2.7])
            {
              cube(size = [20, 2, 1.5], center = false);
            }
          }
          color(c = "goldenrod", alpha = 1)
          {
            translate(v = [-10, -57, 2.7])
            {
              cube(size = [20, 2, 1.5], center = false);
            }
          }
          color(c = "black", alpha = 1)
          {
            translate(v = [-10.16, -3.27, 5.4])
            {
              cube(size = [20.32, 2.54, 2.5], center = false);
            }
          }
          color(c = "gold", alpha = 1)
          {
            union()
            {
              translate(v = [-9.21, -2.32, 2.7])
              {
                cube(size = [0.6400000000000006, 0.6399999999999999, 11.2], center = false);
              }
              translate(v = [-6.67, -2.32, 2.7])
              {
                cube(size = [0.6400000000000006, 0.6399999999999999, 11.2], center = false);
              }
              translate(v = [-4.13, -2.32, 2.7])
              {
                cube(size = [0.6399999999999997, 0.6399999999999999, 11.2], center = false);
              }
              translate(v = [-1.59, -2.32, 2.7])
              {
                cube(size = [0.6400000000000001, 0.6399999999999999, 11.2], center = false);
              }
              translate(v = [0.95, -2.32, 2.7])
              {
                cube(size = [0.6400000000000001, 0.6399999999999999, 11.2], center = false);
              }
              translate(v = [3.49, -2.32, 2.7])
              {
                cube(size = [0.6399999999999997, 0.6399999999999999, 11.2], center = false);
              }
              translate(v = [6.029999999999999, -2.32, 2.7])
              {
                cube(size = [0.6400000000000006, 0.6399999999999999, 11.2], center = false);
              }
              translate(v = [8.57, -2.32, 2.7])
              {
                cube(size = [0.6400000000000006, 0.6399999999999999, 11.2], center = false);
              }
            }
          }
          color(c = "dimgray", alpha = 1)
          {
            union()
            {
              translate(v = [-10.11, -3.22, 7.9])
              {
                cube(size = [2.4399999999999986, 2.4400000000000004, 13.999999999999998], center = false);
              }
              translate(v = [-7.569999999999999, -3.22, 7.9])
              {
                cube(size = [2.4399999999999995, 2.4400000000000004, 13.999999999999998], center = false);
              }
              translate(v = [-5.03, -3.22, 7.9])
              {
                cube(size = [2.4400000000000004, 2.4400000000000004, 13.999999999999998], center = false);
              }
              translate(v = [-2.49, -3.22, 7.9])
              {
                cube(size = [2.4400000000000004, 2.4400000000000004, 13.999999999999998], center = false);
              }
              translate(v = [0.05, -3.22, 7.9])
              {
                cube(size = [2.4400000000000004, 2.4400000000000004, 13.999999999999998], center = false);
              }
              translate(v = [2.59, -3.22, 7.9])
              {
                cube(size = [2.4400000000000004, 2.4400000000000004, 13.999999999999998], center = false);
              }
              translate(v = [5.13, -3.22, 7.9])
              {
                cube(size = [2.4399999999999995, 2.4400000000000004, 13.999999999999998], center = false);
              }
              translate(v = [7.670000000000001, -3.22, 7.9])
              {
                cube(size = [2.4399999999999986, 2.4400000000000004, 13.999999999999998], center = false);
              }
            }
          }
        }
      }
    }
    translate(v = [-50, -50, -1])
    {
      cube(size = [100, 100, 9.9], center = false);
    }
  }
  color(c = "green", alpha = 1)
  {
    translate(v = [-27.7, -10.5, 10.9])
    {
      cube(size = [51, 21, 1], center = false);
    }
  }
  color(c = "black", alpha = 1)
  {
    translate(v = [-26.7, -10.16, 11.9])
    {
      cube(size = [49, 2.539999999999999, 2.5], center = false);
    }
  }
  color(c = "black", alpha = 1)
  {
    translate(v = [-26.7, 7.620000000000001, 11.9])
    {
      cube(size = [49, 2.539999999999999, 2.5], center = false);
    }
  }
  color(c = "silver", alpha = 1)
  {
    translate(v = [-29, -4, 8.3])
    {
      cube(size = [5.300000000000001, 8, 2.5999999999999996], center = false);
    }
  }
  color(c = "silver", alpha = 1)
  {
    difference()
    {
      union()
      {
        translate(v = [0, 0, 14.4])
        {
          linear_extrude(height = 1.1999999999999993, center = false, convexity = undef, twist = undef, slices = undef, scale = 1, $fn = 20)
          {
            offset(r = 3, delta = undef, chamfer = undef)
            {
              square(size = [55.800000000000004, 35.300000000000004], center = true);
            }
          }
        }
        union()
        {
          union()
          {
            translate(v = [-4, -18.75, 7.4])
            {
              cube(size = [8, 1, 7.01], center = false);
            }
            hull()
            {
              translate(v = [-4, -18.75, 7.4])
              {
                cube(size = [8, 0.010000000000001563, 0.009999999999999787], center = false);
              }
              translate(v = [-4, -19.55, 8.59])
              {
                cube(size = [8, 0.8100000000000023, 0.009999999999999787], center = false);
              }
            }
          }
          union()
          {
            translate(v = [-4, 17.75, 7.4])
            {
              cube(size = [8, 1, 7.01], center = false);
            }
            hull()
            {
              translate(v = [-4, 18.74, 7.4])
              {
                cube(size = [8, 0.010000000000001563, 0.009999999999999787], center = false);
              }
              translate(v = [-4, 18.74, 8.59])
              {
                cube(size = [8, 0.8100000000000023, 0.009999999999999787], center = false);
              }
            }
          }
        }
        union()
        {
          translate(v = [-25.7, -5.6, 11.9])
          {
            cylinder(h = 2.51, r = 2.2, center = false);
          }
          translate(v = [-25.7, 5.6, 11.9])
          {
            cylinder(h = 2.51, r = 2.2, center = false);
          }
          translate(v = [21.3, -5.6, 11.9])
          {
            cylinder(h = 2.51, r = 2.2, center = false);
          }
          translate(v = [21.3, 5.6, 11.9])
          {
            cylinder(h = 2.51, r = 2.2, center = false);
          }
        }
        union()
        {
          translate(v = [-25.7, -5.6, 10.3])
          {
            cylinder(h = 1.6099999999999994, r = 0.8, center = false);
          }
        }
        union()
        {
          translate(v = [-25.7, 5.6, 10.3])
          {
            cylinder(h = 1.6099999999999994, r = 0.8, center = false);
          }
        }
        union()
        {
          translate(v = [21.3, -5.6, 9.4])
          {
            cylinder(h = 2.51, r = 0.8, center = false);
          }
        }
        union()
        {
          translate(v = [21.3, 5.6, 9.4])
          {
            cylinder(h = 2.51, r = 0.8, center = false);
          }
        }
      }
      hull()
      {
        translate(v = [10.15, -14.775, 13.4])
        {
          cylinder(h = 3.2, r = 3.5, center = false);
        }
        translate(v = [19.15, -14.775, 13.4])
        {
          cylinder(h = 3.2, r = 3.5, center = false);
        }
      }
      union()
      {
        translate(v = [-23.506148721743873, -13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-19.588457268119896, -13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-15.670765814495915, -13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-11.753074360871937, -13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-7.835382907247958, -13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-3.917691453623979, -13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [0, -13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [3.917691453623979, -13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-17.629611541307906, -10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-13.711920087683925, -10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-9.794228634059948, -10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-5.876537180435968, -10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-1.9588457268119894, -10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [1.9588457268119894, -10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-19.588457268119896, -6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-15.670765814495915, -6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-11.753074360871937, -6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-7.835382907247958, -6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-3.917691453623979, -6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [0, -6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [3.917691453623979, -6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [7.835382907247958, -6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [11.753074360871937, -6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [15.670765814495915, -6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-17.629611541307906, -3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-13.711920087683925, -3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-9.794228634059948, -3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-5.876537180435968, -3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-1.9588457268119894, -3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [1.9588457268119894, -3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [5.876537180435968, -3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [9.794228634059948, -3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [13.711920087683925, -3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-23.506148721743873, 0, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-19.588457268119896, 0, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-15.670765814495915, 0, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-11.753074360871937, 0, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-7.835382907247958, 0, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-3.917691453623979, 0, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [3.917691453623979, 0, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [7.835382907247958, 0, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [11.753074360871937, 0, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [15.670765814495915, 0, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [19.588457268119896, 0, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [23.506148721743873, 0, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-17.629611541307906, 3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-13.711920087683925, 3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-9.794228634059948, 3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-5.876537180435968, 3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-1.9588457268119894, 3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [1.9588457268119894, 3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [5.876537180435968, 3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [9.794228634059948, 3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [13.711920087683925, 3.3928203230275504, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-19.588457268119896, 6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-15.670765814495915, 6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-11.753074360871937, 6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-7.835382907247958, 6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-3.917691453623979, 6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [0, 6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [3.917691453623979, 6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [7.835382907247958, 6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [11.753074360871937, 6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [15.670765814495915, 6.785640646055101, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-17.629611541307906, 10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-13.711920087683925, 10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-9.794228634059948, 10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-5.876537180435968, 10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-1.9588457268119894, 10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [1.9588457268119894, 10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [5.876537180435968, 10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [9.794228634059948, 10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [13.711920087683925, 10.178460969082652, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-23.506148721743873, 13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-19.588457268119896, 13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-15.670765814495915, 13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-11.753074360871937, 13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-7.835382907247958, 13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-3.917691453623979, 13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [0, 13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [3.917691453623979, 13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [7.835382907247958, 13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [11.753074360871937, 13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [15.670765814495915, 13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [19.588457268119896, 13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [23.506148721743873, 13.571281292110202, 13.4])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.200000000000001, r = 1.8, center = false, $fn = 6);
          }
        }
      }
    }
  }
}
