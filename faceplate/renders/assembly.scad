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
          translate(v = [0, -40.125, 0])
          {
            linear_extrude(height = 5.4, center = false, convexity = undef, twist = undef, slices = undef, scale = 1, $fn = 20)
            {
              offset(r = 3, delta = undef, chamfer = undef)
              {
                square(size = [48.1, 90.85], center = true);
              }
            }
          }
          union()
          {
            translate(v = [-6.875, -81.9, 5.390000000000001])
            {
              cylinder(h = 0.6099999999999997, r = 2.5, center = false);
            }
            translate(v = [-6.875, -66.1, 5.390000000000001])
            {
              cylinder(h = 0.6099999999999997, r = 2.5, center = false);
            }
            translate(v = [6.875, -81.9, 5.390000000000001])
            {
              cylinder(h = 0.6099999999999997, r = 2.5, center = false);
            }
            translate(v = [6.875, -66.1, 5.390000000000001])
            {
              cylinder(h = 0.6099999999999997, r = 2.5, center = false);
            }
          }
        }
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
        union()
        {
          translate(v = [-15.75, -3, 1.2])
          {
            cylinder(h = 5.4, r = 0.85, center = false);
          }
          translate(v = [-15.75, -55, 1.2])
          {
            cylinder(h = 5.4, r = 0.85, center = false);
          }
          translate(v = [15.75, -3, 1.2])
          {
            cylinder(h = 5.4, r = 0.85, center = false);
          }
          translate(v = [15.75, -55, 1.2])
          {
            cylinder(h = 5.4, r = 0.85, center = false);
          }
        }
        union()
        {
          translate(v = [0, -74, -1])
          {
            cylinder(h = 7.4, r = 3.8, center = false);
          }
          translate(v = [-6.55, -80.55, 1])
          {
            cube(size = [13.1, 13.099999999999994, 6], center = false);
          }
        }
        union()
        {
          translate(v = [-6.875, -81.9, 1.2])
          {
            cylinder(h = 6, r = 0.85, center = false);
          }
          translate(v = [-6.875, -66.1, 1.2])
          {
            cylinder(h = 6, r = 0.85, center = false);
          }
          translate(v = [6.875, -81.9, 1.2])
          {
            cylinder(h = 6, r = 0.85, center = false);
          }
          translate(v = [6.875, -66.1, 1.2])
          {
            cylinder(h = 6, r = 0.85, center = false);
          }
        }
      }
      difference()
      {
        union()
        {
          translate(v = [-23.3, 4.550000000000001, 5.390000000000001])
          {
            cylinder(h = 20.01, r = 3.25, center = false);
          }
          translate(v = [23.3, 4.550000000000001, 5.390000000000001])
          {
            cylinder(h = 20.01, r = 3.25, center = false);
          }
          translate(v = [-23.3, -84.8, 5.390000000000001])
          {
            cylinder(h = 20.01, r = 3.25, center = false);
          }
          translate(v = [23.3, -84.8, 5.390000000000001])
          {
            cylinder(h = 20.01, r = 3.25, center = false);
          }
        }
        translate(v = [-23.3, 4.550000000000001, 6.4])
        {
          cylinder(h = 20, r = 1.25, center = false);
        }
        translate(v = [23.3, 4.550000000000001, 6.4])
        {
          cylinder(h = 20, r = 1.25, center = false);
        }
        translate(v = [-23.3, -84.8, 6.4])
        {
          cylinder(h = 20, r = 1.25, center = false);
        }
        translate(v = [23.3, -84.8, 6.4])
        {
          cylinder(h = 20, r = 1.25, center = false);
        }
      }
    }
  }
  union()
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
        translate(v = [-10, -57, 2.7])
        {
          cube(size = [20, 2, 1.5], center = false);
        }
      }
      color(c = "goldenrod", alpha = 1)
      {
        translate(v = [-10, -3, 2.7])
        {
          cube(size = [20, 2, 1.5], center = false);
        }
      }
      color(c = "black", alpha = 1)
      {
        translate(v = [-10.16, -57.27, 5.4])
        {
          cube(size = [20.32, 2.5400000000000063, 2.5], center = false);
        }
      }
      color(c = "gold", alpha = 1)
      {
        union()
        {
          translate(v = [-9.21, -56.32, 2.7])
          {
            cube(size = [0.6400000000000006, 0.6400000000000006, 11.2], center = false);
          }
          translate(v = [-6.67, -56.32, 2.7])
          {
            cube(size = [0.6400000000000006, 0.6400000000000006, 11.2], center = false);
          }
          translate(v = [-4.13, -56.32, 2.7])
          {
            cube(size = [0.6399999999999997, 0.6400000000000006, 11.2], center = false);
          }
          translate(v = [-1.59, -56.32, 2.7])
          {
            cube(size = [0.6400000000000001, 0.6400000000000006, 11.2], center = false);
          }
          translate(v = [0.95, -56.32, 2.7])
          {
            cube(size = [0.6400000000000001, 0.6400000000000006, 11.2], center = false);
          }
          translate(v = [3.49, -56.32, 2.7])
          {
            cube(size = [0.6399999999999997, 0.6400000000000006, 11.2], center = false);
          }
          translate(v = [6.029999999999999, -56.32, 2.7])
          {
            cube(size = [0.6400000000000006, 0.6400000000000006, 11.2], center = false);
          }
          translate(v = [8.57, -56.32, 2.7])
          {
            cube(size = [0.6400000000000006, 0.6400000000000006, 11.2], center = false);
          }
        }
      }
      color(c = "dimgray", alpha = 1)
      {
        union()
        {
          translate(v = [-10.11, -57.220000000000006, 7.9])
          {
            cube(size = [2.4399999999999986, 2.440000000000012, 13.999999999999998], center = false);
          }
          translate(v = [-7.569999999999999, -57.220000000000006, 7.9])
          {
            cube(size = [2.4399999999999995, 2.440000000000012, 13.999999999999998], center = false);
          }
          translate(v = [-5.03, -57.220000000000006, 7.9])
          {
            cube(size = [2.4400000000000004, 2.440000000000012, 13.999999999999998], center = false);
          }
          translate(v = [-2.49, -57.220000000000006, 7.9])
          {
            cube(size = [2.4400000000000004, 2.440000000000012, 13.999999999999998], center = false);
          }
          translate(v = [0.05, -57.220000000000006, 7.9])
          {
            cube(size = [2.4400000000000004, 2.440000000000012, 13.999999999999998], center = false);
          }
          translate(v = [2.59, -57.220000000000006, 7.9])
          {
            cube(size = [2.4400000000000004, 2.440000000000012, 13.999999999999998], center = false);
          }
          translate(v = [5.13, -57.220000000000006, 7.9])
          {
            cube(size = [2.4399999999999995, 2.440000000000012, 13.999999999999998], center = false);
          }
          translate(v = [7.670000000000001, -57.220000000000006, 7.9])
          {
            cube(size = [2.4399999999999986, 2.440000000000012, 13.999999999999998], center = false);
          }
        }
      }
    }
    union()
    {
      color(c = "silver", alpha = 1)
      {
        translate(v = [-6.25, -80.25, 1])
        {
          cube(size = [12.5, 12.5, 5], center = false);
        }
      }
      color(c = "darkgray", alpha = 1)
      {
        translate(v = [0, -74, -5])
        {
          cylinder(h = 6, r = 3.5, center = false);
        }
      }
      color(c = "lightgray", alpha = 1)
      {
        translate(v = [0, -74, -20])
        {
          cylinder(h = 15, r = 3, center = false);
        }
      }
      color(c = "midnightblue", alpha = 1)
      {
        translate(v = [-15, -83.25, 6])
        {
          cube(size = [30, 18.5, 1.5999999999999996], center = false);
        }
      }
      color(c = "black", alpha = 1)
      {
        translate(v = [11.73, -80.35, 7.6])
        {
          cube(size = [2.539999999999999, 12.699999999999989, 2.5], center = false);
        }
      }
      color(c = "gold", alpha = 1)
      {
        union()
        {
          translate(v = [12.68, -79.39999999999999, 5])
          {
            cube(size = [0.6400000000000006, 0.6399999999999864, 6.42], center = false);
          }
          translate(v = [12.68, -76.86, 5])
          {
            cube(size = [0.6400000000000006, 0.6399999999999864, 6.42], center = false);
          }
          translate(v = [12.68, -74.32, 5])
          {
            cube(size = [0.6400000000000006, 0.6399999999999864, 6.42], center = false);
          }
          translate(v = [12.68, -71.77999999999999, 5])
          {
            cube(size = [0.6400000000000006, 0.6399999999999864, 6.42], center = false);
          }
          translate(v = [12.68, -69.24, 5])
          {
            cube(size = [0.6400000000000006, 0.6399999999999864, 6.42], center = false);
          }
        }
      }
      color(c = "gold", alpha = 1)
      {
        union()
        {
          translate(v = [12.68, -79.39999999999999, 10.78])
          {
            cube(size = [6.32, 0.6399999999999864, 0.6400000000000006], center = false);
          }
          translate(v = [12.68, -76.86, 10.78])
          {
            cube(size = [6.32, 0.6399999999999864, 0.6400000000000006], center = false);
          }
          translate(v = [12.68, -74.32, 10.78])
          {
            cube(size = [6.32, 0.6399999999999864, 0.6400000000000006], center = false);
          }
          translate(v = [12.68, -71.77999999999999, 10.78])
          {
            cube(size = [6.32, 0.6399999999999864, 0.6400000000000006], center = false);
          }
          translate(v = [12.68, -69.24, 10.78])
          {
            cube(size = [6.32, 0.6399999999999864, 0.6400000000000006], center = false);
          }
        }
      }
      color(c = "dimgray", alpha = 1)
      {
        union()
        {
          translate(v = [13, -80.3, 9.83])
          {
            cube(size = [14, 2.4399999999999977, 2.539999999999999], center = false);
          }
          translate(v = [13, -77.76, 9.83])
          {
            cube(size = [14, 2.4399999999999977, 2.539999999999999], center = false);
          }
          translate(v = [13, -75.22, 9.83])
          {
            cube(size = [14, 2.4399999999999977, 2.539999999999999], center = false);
          }
          translate(v = [13, -72.67999999999999, 9.83])
          {
            cube(size = [14, 2.4399999999999977, 2.539999999999999], center = false);
          }
          translate(v = [13, -70.14, 9.83])
          {
            cube(size = [14, 2.4399999999999977, 2.539999999999999], center = false);
          }
        }
      }
    }
  }
  translate(v = [0, 0, 25.4])
  {
    union()
    {
      color(c = "silver", alpha = 1)
      {
        difference()
        {
          union()
          {
            translate(v = [0, -40.125, 0])
            {
              linear_extrude(height = 2.5, center = false, convexity = undef, twist = undef, slices = undef, scale = 1, $fn = 20)
              {
                offset(r = 3, delta = undef, chamfer = undef)
                {
                  square(size = [48.1, 90.85], center = true);
                }
              }
            }
            union()
            {
              translate(v = [-25.3, -53.25, 2.49])
              {
                cube(size = [2, 27.5, 5.01], center = false);
              }
              translate(v = [23.3, -53.25, 2.49])
              {
                cube(size = [2, 27.5, 5.01], center = false);
              }
            }
            translate(v = [-23.5, -18.1, 2.49])
            {
              cylinder(h = 3.01, r = 2.65, center = false);
            }
            translate(v = [-23.5, -6.9, 2.49])
            {
              cylinder(h = 3.01, r = 2.65, center = false);
            }
            translate(v = [23.5, -18.1, 2.49])
            {
              cylinder(h = 3.01, r = 2.65, center = false);
            }
            translate(v = [23.5, -6.9, 2.49])
            {
              cylinder(h = 3.01, r = 2.65, center = false);
            }
            translate(v = [-17.5, -81.5, 2.49])
            {
              cylinder(h = 5.01, r = 3.05, center = false);
            }
            translate(v = [-17.5, -58.5, 2.49])
            {
              cylinder(h = 5.01, r = 3.05, center = false);
            }
            translate(v = [17.5, -81.5, 2.49])
            {
              cylinder(h = 5.01, r = 3.05, center = false);
            }
            translate(v = [17.5, -58.5, 2.49])
            {
              cylinder(h = 5.01, r = 3.05, center = false);
            }
          }
          translate(v = [-23.3, 4.550000000000001, -1])
          {
            cylinder(h = 4.5, r = 1.7, center = false);
          }
          translate(v = [23.3, 4.550000000000001, -1])
          {
            cylinder(h = 4.5, r = 1.7, center = false);
          }
          translate(v = [-23.3, -84.8, -1])
          {
            cylinder(h = 4.5, r = 1.7, center = false);
          }
          translate(v = [23.3, -84.8, -1])
          {
            cylinder(h = 4.5, r = 1.7, center = false);
          }
          translate(v = [-2.25, -55.55, -1])
          {
            cube(size = [4.5, 1.5, 100], center = false);
          }
          translate(v = [-2.25, -24.95, -1])
          {
            cube(size = [4.5, 1.5, 100], center = false);
          }
          translate(v = [-25.7, -22.96, -1])
          {
            cube(size = [51.4, 3.1400000000000006, 100], center = false);
          }
          translate(v = [-25.7, -5.18, -1])
          {
            cube(size = [51.4, 3.1400000000000006, 100], center = false);
          }
          translate(v = [-23.5, -18.1, 0.8])
          {
            cylinder(h = 5.5, r = 0.85, center = false);
          }
          translate(v = [-23.5, -6.9, 0.8])
          {
            cylinder(h = 5.5, r = 0.85, center = false);
          }
          translate(v = [23.5, -18.1, 0.8])
          {
            cylinder(h = 5.5, r = 0.85, center = false);
          }
          translate(v = [23.5, -6.9, 0.8])
          {
            cylinder(h = 5.5, r = 0.85, center = false);
          }
          translate(v = [-17.5, -81.5, 0.8])
          {
            cylinder(h = 7.5, r = 1.25, center = false);
          }
          translate(v = [-17.5, -58.5, 0.8])
          {
            cylinder(h = 7.5, r = 1.25, center = false);
          }
          translate(v = [17.5, -81.5, 0.8])
          {
            cylinder(h = 7.5, r = 1.25, center = false);
          }
          translate(v = [17.5, -58.5, 0.8])
          {
            cylinder(h = 7.5, r = 1.25, center = false);
          }
        }
      }
      union()
      {
        union()
        {
          color(c = "green", alpha = 1)
          {
            translate(v = [-25.5, -23, 5.5])
            {
              cube(size = [51, 21, 1], center = false);
            }
          }
          color(c = "silver", alpha = 1)
          {
            translate(v = [20.5, -16.5, 6.5])
            {
              cube(size = [6, 8, 2.5999999999999996], center = false);
            }
          }
          color(c = "black", alpha = 1)
          {
            translate(v = [-3.5, -16, 6.5])
            {
              cube(size = [7, 7, 1], center = false);
            }
          }
          color(c = "silver", alpha = 1)
          {
            translate(v = [-21.5, -17.5, 6.5])
            {
              cube(size = [10, 10, 1.5999999999999996], center = false);
            }
          }
          color(c = "black", alpha = 1)
          {
            translate(v = [-25.4, -22.66, 3])
            {
              cube(size = [50.8, 2.539999999999999, 2.5], center = false);
            }
          }
          color(c = "gold", alpha = 1)
          {
            union()
            {
              translate(v = [-24.45, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [-21.91, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [-19.37, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [-16.830000000000002, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [-14.290000000000001, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [-11.75, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [-9.21, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [-6.67, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [-4.13, -21.71, -3])
              {
                cube(size = [0.6399999999999997, 0.6400000000000006, 11], center = false);
              }
              translate(v = [-1.59, -21.71, -3])
              {
                cube(size = [0.6400000000000001, 0.6400000000000006, 11], center = false);
              }
              translate(v = [0.95, -21.71, -3])
              {
                cube(size = [0.6400000000000001, 0.6400000000000006, 11], center = false);
              }
              translate(v = [3.49, -21.71, -3])
              {
                cube(size = [0.6399999999999997, 0.6400000000000006, 11], center = false);
              }
              translate(v = [6.029999999999999, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [8.57, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [11.11, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [13.65, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [16.19, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [18.73, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [21.27, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
              translate(v = [23.81, -21.71, -3])
              {
                cube(size = [0.6400000000000006, 0.6400000000000006, 11], center = false);
              }
            }
          }
          color(c = "dimgray", alpha = 1)
          {
            union()
            {
              translate(v = [-25.349999999999998, -22.61, -11])
              {
                cube(size = [2.4399999999999977, 2.4399999999999977, 14], center = false);
              }
              translate(v = [-22.81, -22.61, -11])
              {
                cube(size = [2.4399999999999977, 2.4399999999999977, 14], center = false);
              }
              translate(v = [-20.27, -22.61, -11])
              {
                cube(size = [2.4399999999999977, 2.4399999999999977, 14], center = false);
              }
              translate(v = [-17.73, -22.61, -11])
              {
                cube(size = [2.4399999999999977, 2.4399999999999977, 14], center = false);
              }
              translate(v = [-15.19, -22.61, -11])
              {
                cube(size = [2.4399999999999977, 2.4399999999999977, 14], center = false);
              }
              translate(v = [-12.649999999999999, -22.61, -11])
              {
                cube(size = [2.4399999999999977, 2.4399999999999977, 14], center = false);
              }
              translate(v = [-10.11, -22.61, -11])
              {
                cube(size = [2.4399999999999986, 2.4399999999999977, 14], center = false);
              }
              translate(v = [-7.569999999999999, -22.61, -11])
              {
                cube(size = [2.4399999999999995, 2.4399999999999977, 14], center = false);
              }
              translate(v = [-5.03, -22.61, -11])
              {
                cube(size = [2.4400000000000004, 2.4399999999999977, 14], center = false);
              }
              translate(v = [-2.49, -22.61, -11])
              {
                cube(size = [2.4400000000000004, 2.4399999999999977, 14], center = false);
              }
              translate(v = [0.05, -22.61, -11])
              {
                cube(size = [2.4400000000000004, 2.4399999999999977, 14], center = false);
              }
              translate(v = [2.59, -22.61, -11])
              {
                cube(size = [2.4400000000000004, 2.4399999999999977, 14], center = false);
              }
              translate(v = [5.13, -22.61, -11])
              {
                cube(size = [2.4399999999999995, 2.4399999999999977, 14], center = false);
              }
              translate(v = [7.670000000000001, -22.61, -11])
              {
                cube(size = [2.4399999999999986, 2.4399999999999977, 14], center = false);
              }
              translate(v = [10.21, -22.61, -11])
              {
                cube(size = [2.4399999999999977, 2.4399999999999977, 14], center = false);
              }
              translate(v = [12.750000000000002, -22.61, -11])
              {
                cube(size = [2.4399999999999977, 2.4399999999999977, 14], center = false);
              }
              translate(v = [15.290000000000003, -22.61, -11])
              {
                cube(size = [2.4399999999999977, 2.4399999999999977, 14], center = false);
              }
              translate(v = [17.830000000000002, -22.61, -11])
              {
                cube(size = [2.4399999999999977, 2.4399999999999977, 14], center = false);
              }
              translate(v = [20.37, -22.61, -11])
              {
                cube(size = [2.4399999999999977, 2.4399999999999977, 14], center = false);
              }
              translate(v = [22.91, -22.61, -11])
              {
                cube(size = [2.4399999999999977, 2.4399999999999977, 14], center = false);
              }
            }
          }
          color(c = "black", alpha = 1)
          {
            translate(v = [-25.4, -4.879999999999999, 3])
            {
              cube(size = [50.8, 2.5399999999999996, 2.5], center = false);
            }
          }
          color(c = "gold", alpha = 1)
          {
            union()
            {
              translate(v = [-24.45, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [-21.91, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [-19.37, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [-16.830000000000002, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [-14.290000000000001, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [-11.75, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [-9.21, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [-6.67, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [-4.13, -3.9299999999999993, -3])
              {
                cube(size = [0.6399999999999997, 0.6399999999999997, 11], center = false);
              }
              translate(v = [-1.59, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000001, 0.6399999999999997, 11], center = false);
              }
              translate(v = [0.95, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000001, 0.6399999999999997, 11], center = false);
              }
              translate(v = [3.49, -3.9299999999999993, -3])
              {
                cube(size = [0.6399999999999997, 0.6399999999999997, 11], center = false);
              }
              translate(v = [6.029999999999999, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [8.57, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [11.11, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [13.65, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [16.19, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [18.73, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [21.27, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
              translate(v = [23.81, -3.9299999999999993, -3])
              {
                cube(size = [0.6400000000000006, 0.6399999999999997, 11], center = false);
              }
            }
          }
          color(c = "dimgray", alpha = 1)
          {
            union()
            {
              translate(v = [-25.349999999999998, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999977, 2.44, 14], center = false);
              }
              translate(v = [-22.81, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999977, 2.44, 14], center = false);
              }
              translate(v = [-20.27, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999977, 2.44, 14], center = false);
              }
              translate(v = [-17.73, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999977, 2.44, 14], center = false);
              }
              translate(v = [-15.19, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999977, 2.44, 14], center = false);
              }
              translate(v = [-12.649999999999999, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999977, 2.44, 14], center = false);
              }
              translate(v = [-10.11, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999986, 2.44, 14], center = false);
              }
              translate(v = [-7.569999999999999, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999995, 2.44, 14], center = false);
              }
              translate(v = [-5.03, -4.829999999999999, -11])
              {
                cube(size = [2.4400000000000004, 2.44, 14], center = false);
              }
              translate(v = [-2.49, -4.829999999999999, -11])
              {
                cube(size = [2.4400000000000004, 2.44, 14], center = false);
              }
              translate(v = [0.05, -4.829999999999999, -11])
              {
                cube(size = [2.4400000000000004, 2.44, 14], center = false);
              }
              translate(v = [2.59, -4.829999999999999, -11])
              {
                cube(size = [2.4400000000000004, 2.44, 14], center = false);
              }
              translate(v = [5.13, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999995, 2.44, 14], center = false);
              }
              translate(v = [7.670000000000001, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999986, 2.44, 14], center = false);
              }
              translate(v = [10.21, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999977, 2.44, 14], center = false);
              }
              translate(v = [12.750000000000002, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999977, 2.44, 14], center = false);
              }
              translate(v = [15.290000000000003, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999977, 2.44, 14], center = false);
              }
              translate(v = [17.830000000000002, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999977, 2.44, 14], center = false);
              }
              translate(v = [20.37, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999977, 2.44, 14], center = false);
              }
              translate(v = [22.91, -4.829999999999999, -11])
              {
                cube(size = [2.4399999999999977, 2.44, 14], center = false);
              }
            }
          }
        }
        union()
        {
          color(c = "navy", alpha = 1)
          {
            translate(v = [-20, -84, 7.5])
            {
              cube(size = [40, 28, 1.5999999999999996], center = false);
            }
          }
          color(c = "deepskyblue", alpha = 1)
          {
            translate(v = [-5, -63.5, 9.1])
            {
              cube(size = [10, 7.5, 10.000000000000002], center = false);
            }
          }
          color(c = "black", alpha = 1)
          {
            translate(v = [-12, -73, 9.1])
            {
              cube(size = [10, 7.5, 1.5], center = false);
            }
          }
          color(c = "black", alpha = 1)
          {
            translate(v = [6, -71, 9.1])
            {
              cube(size = [5, 4, 1.5], center = false);
            }
          }
          color(c = "silver", alpha = 1)
          {
            translate(v = [-12, -79, 9.1])
            {
              cube(size = [11, 4.5, 3.5], center = false);
            }
          }
          color(c = "black", alpha = 1)
          {
            translate(v = [-8.89, -83.27, 9.1])
            {
              cube(size = [17.78, 2.539999999999992, 2.5], center = false);
            }
          }
          color(c = "gold", alpha = 1)
          {
            union()
            {
              translate(v = [-7.94, -82.32, 6])
              {
                cube(size = [0.6400000000000006, 0.6399999999999864, 11.600000000000001], center = false);
              }
              translate(v = [-5.4, -82.32, 6])
              {
                cube(size = [0.6400000000000006, 0.6399999999999864, 11.600000000000001], center = false);
              }
              translate(v = [-2.86, -82.32, 6])
              {
                cube(size = [0.6399999999999997, 0.6399999999999864, 11.600000000000001], center = false);
              }
              translate(v = [-0.32, -82.32, 6])
              {
                cube(size = [0.64, 0.6399999999999864, 11.600000000000001], center = false);
              }
              translate(v = [2.22, -82.32, 6])
              {
                cube(size = [0.6399999999999997, 0.6399999999999864, 11.600000000000001], center = false);
              }
              translate(v = [4.76, -82.32, 6])
              {
                cube(size = [0.6400000000000006, 0.6399999999999864, 11.600000000000001], center = false);
              }
              translate(v = [7.3, -82.32, 6])
              {
                cube(size = [0.6400000000000006, 0.6399999999999864, 11.600000000000001], center = false);
              }
            }
          }
        }
        union()
        {
          color(c = "dimgray", alpha = 1)
          {
            translate(v = [-23, -53.25, 2.5])
            {
              cube(size = [46, 27.5, 14], center = false);
            }
          }
          color(c = "white", alpha = 1)
          {
            translate(v = [-2, -54.75, 16.5])
            {
              cube(size = [4, 30.5, 1], center = false);
            }
          }
        }
      }
    }
  }
}
