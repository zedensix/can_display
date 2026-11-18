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
          difference()
          {
            union()
            {
              translate(v = [0, 0, 0])
              {
                cylinder(h = 2.2, r = 36.5, center = false);
              }
              translate(v = [0, 0, 2.1900000000000004])
              {
                cylinder(h = 5.01, r1 = 36.5, r2 = 41.5, center = false);
              }
              translate(v = [0, 0, 7.19])
              {
                cylinder(h = 17.31, r = 41.5, center = false);
              }
              translate(v = [-43, -1.5, 22])
              {
                cube(size = [2, 3, 2.5], center = false);
              }
              translate(v = [41, -1.5, 22])
              {
                cube(size = [2, 3, 2.5], center = false);
              }
            }
            union()
            {
              intersection()
              {
                union()
                {
                  translate(v = [0, 0, 5.4])
                  {
                    cylinder(h = 1.7999999999999998, r1 = 37.7, r2 = 39.5, center = false);
                  }
                  translate(v = [0, 0, 7.19])
                  {
                    cylinder(h = 17.32, r = 39.5, center = false);
                  }
                }
                translate(v = [0, 0, 5.4])
                {
                  cylinder(h = 19.11, r1 = 39.3, r2 = 20.2, center = false);
                }
              }
              translate(v = [0, 0, 24.49])
              {
                cylinder(h = 5.010000000000002, r = 20.2, center = false);
              }
            }
          }
          translate(v = [0, 50, 0])
          {
            union()
            {
              translate(v = [-4.875, -81.9, 5.390000000000001])
              {
                cylinder(h = 0.6099999999999997, r = 2.5, center = false);
              }
              translate(v = [-4.875, -66.1, 5.390000000000001])
              {
                cylinder(h = 0.6099999999999997, r = 2.5, center = false);
              }
              translate(v = [8.875, -81.9, 5.390000000000001])
              {
                cylinder(h = 0.6099999999999997, r = 2.5, center = false);
              }
              translate(v = [8.875, -66.1, 5.390000000000001])
              {
                cylinder(h = 0.6099999999999997, r = 2.5, center = false);
              }
            }
          }
        }
        translate(v = [29, 2, 0])
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
              union()
              {
                translate(v = [-15.25, -55.5, 1.2])
                {
                  cylinder(h = 5.4, r = 0.85, center = false);
                }
                translate(v = [-15.25, -2.5, 1.2])
                {
                  cylinder(h = 5.4, r = 0.85, center = false);
                }
                translate(v = [15.25, -55.5, 1.2])
                {
                  cylinder(h = 5.4, r = 0.85, center = false);
                }
                translate(v = [15.25, -2.5, 1.2])
                {
                  cylinder(h = 5.4, r = 0.85, center = false);
                }
              }
            }
          }
        }
        translate(v = [0, 50, 0])
        {
          union()
          {
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
              translate(v = [-4.875, -81.9, 1.2])
              {
                cylinder(h = 6, r = 0.85, center = false);
              }
              translate(v = [-4.875, -66.1, 1.2])
              {
                cylinder(h = 6, r = 0.85, center = false);
              }
              translate(v = [8.875, -81.9, 1.2])
              {
                cylinder(h = 6, r = 0.85, center = false);
              }
              translate(v = [8.875, -66.1, 1.2])
              {
                cylinder(h = 6, r = 0.85, center = false);
              }
            }
          }
        }
        union()
        {
          union()
          {
            translate(v = [-19.588457268119896, -27.142562584220403, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [19.588457268119896, -27.142562584220403, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-21.547302994931883, -23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-17.629611541307906, -23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [17.629611541307906, -23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [21.547302994931886, -23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-23.506148721743873, -20.356921938165303, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-19.588457268119896, -20.356921938165303, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [19.588457268119896, -20.356921938165303, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [23.506148721743873, -20.356921938165303, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-33.300377355803825, -3.3928203230275504, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [33.30037735580382, -3.3928203230275504, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-33.300377355803825, 3.3928203230275504, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [33.30037735580382, 3.3928203230275504, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-21.547302994931883, 23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-17.629611541307906, 23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-13.711920087683925, 23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-9.794228634059948, 23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-5.876537180435968, 23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-1.9588457268119894, 23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [1.9588457268119894, 23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [5.876537180435968, 23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [9.794228634059948, 23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [13.711920087683925, 23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [17.629611541307906, 23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [21.547302994931886, 23.749742261192853, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-19.588457268119896, 27.142562584220403, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-15.670765814495915, 27.142562584220403, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-11.753074360871937, 27.142562584220403, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-7.835382907247958, 27.142562584220403, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-3.917691453623979, 27.142562584220403, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [0, 27.142562584220403, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [3.917691453623979, 27.142562584220403, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [7.835382907247958, 27.142562584220403, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [11.753074360871937, 27.142562584220403, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [15.670765814495915, 27.142562584220403, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [19.588457268119896, 27.142562584220403, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-13.711920087683925, 30.535382907247953, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-9.794228634059948, 30.535382907247953, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-5.876537180435968, 30.535382907247953, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-1.9588457268119894, 30.535382907247953, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [1.9588457268119894, 30.535382907247953, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [5.876537180435968, 30.535382907247953, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [9.794228634059948, 30.535382907247953, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [13.711920087683925, 30.535382907247953, -1])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 7.4, r = 1.8, center = false, $fn = 6);
              }
            }
          }
          union()
          {
            translate(v = [-3.917691453623979, -33.92820323027551, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [0, -33.92820323027551, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [3.917691453623979, -33.92820323027551, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-13.711920087683925, -30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-9.794228634059948, -30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-5.876537180435968, -30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-1.9588457268119894, -30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [1.9588457268119894, -30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [5.876537180435968, -30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [9.794228634059948, -30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [13.711920087683925, -30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-19.588457268119896, -27.142562584220403, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-15.670765814495915, -27.142562584220403, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-11.753074360871937, -27.142562584220403, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [11.753074360871937, -27.142562584220403, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [15.670765814495915, -27.142562584220403, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [19.588457268119896, -27.142562584220403, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-21.547302994931883, -23.749742261192853, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-17.629611541307906, -23.749742261192853, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [17.629611541307906, -23.749742261192853, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [21.547302994931886, -23.749742261192853, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-27.42384017536785, -20.356921938165303, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-23.506148721743873, -20.356921938165303, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [23.506148721743873, -20.356921938165303, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [27.42384017536785, -20.356921938165303, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-29.38268590217984, -16.964101615137753, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-25.46499444855586, -16.964101615137753, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [25.464994448555863, -16.964101615137753, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [29.38268590217984, -16.964101615137753, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-31.34153162899183, -13.571281292110202, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-27.42384017536785, -13.571281292110202, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [27.42384017536785, -13.571281292110202, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [31.34153162899183, -13.571281292110202, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-29.38268590217984, -10.178460969082652, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [29.38268590217984, -10.178460969082652, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-31.34153162899183, -6.785640646055101, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [31.34153162899183, -6.785640646055101, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-33.300377355803825, -3.3928203230275504, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-29.38268590217984, -3.3928203230275504, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [29.38268590217984, -3.3928203230275504, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [33.30037735580382, -3.3928203230275504, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-31.34153162899183, 0, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [31.34153162899183, 0, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-33.300377355803825, 3.3928203230275504, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-29.38268590217984, 3.3928203230275504, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [29.38268590217984, 3.3928203230275504, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [33.30037735580382, 3.3928203230275504, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-31.34153162899183, 6.785640646055101, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [31.34153162899183, 6.785640646055101, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-29.38268590217984, 10.178460969082652, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [29.38268590217984, 10.178460969082652, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-31.34153162899183, 13.571281292110202, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-27.42384017536785, 13.571281292110202, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [27.42384017536785, 13.571281292110202, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [31.34153162899183, 13.571281292110202, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-29.38268590217984, 16.964101615137753, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-25.46499444855586, 16.964101615137753, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [25.464994448555863, 16.964101615137753, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [29.38268590217984, 16.964101615137753, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-27.42384017536785, 20.356921938165303, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-23.506148721743873, 20.356921938165303, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [23.506148721743873, 20.356921938165303, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [27.42384017536785, 20.356921938165303, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-21.547302994931883, 23.749742261192853, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-17.629611541307906, 23.749742261192853, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [17.629611541307906, 23.749742261192853, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [21.547302994931886, 23.749742261192853, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-19.588457268119896, 27.142562584220403, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-15.670765814495915, 27.142562584220403, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-11.753074360871937, 27.142562584220403, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [11.753074360871937, 27.142562584220403, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [15.670765814495915, 27.142562584220403, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [19.588457268119896, 27.142562584220403, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-13.711920087683925, 30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-9.794228634059948, 30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-5.876537180435968, 30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-1.9588457268119894, 30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [1.9588457268119894, 30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [5.876537180435968, 30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [9.794228634059948, 30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [13.711920087683925, 30.535382907247953, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [-3.917691453623979, 33.92820323027551, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [0, 33.92820323027551, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
            translate(v = [3.917691453623979, 33.92820323027551, 6.4])
            {
              rotate(a = [0, 0, 30], v = undef)
              {
                cylinder(h = 19.1, r = 1.8, center = false, $fn = 6);
              }
            }
          }
        }
      }
      difference()
      {
        translate(v = [0, 0, 24.49])
        {
          cylinder(h = 4.010000000000002, r = 23.2, center = false);
        }
        translate(v = [0, 0, 23.5])
        {
          cylinder(h = 6, r = 20.2, center = false);
        }
        translate(v = [-24.2, 0, 26.5])
        {
          rotate(a = [0, 90, 0], v = undef)
          {
            cylinder(h = 48.4, r = 1.25, center = false);
          }
        }
      }
    }
  }
  translate(v = [29, 2, 0])
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
  translate(v = [0, 50, 0])
  {
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
  color(c = "silver", alpha = 1)
  {
    difference()
    {
      difference()
      {
        union()
        {
          difference()
          {
            translate(v = [0, 0, 24.5])
            {
              cylinder(h = 109.10000000000002, r = 26, center = false);
            }
            translate(v = [0, 0, 23.5])
            {
              cylinder(h = 9.009999999999998, r = 23.5, center = false);
            }
            translate(v = [0, 0, 32.5])
            {
              cylinder(h = 98.60000000000002, r = 24.4, center = false);
            }
          }
          translate(v = [-24.9, -3, 32.5])
          {
            cube(size = [1.8999999999999986, 1.2, 98.61000000000001], center = false);
          }
          translate(v = [-24.9, 1.8, 32.5])
          {
            cube(size = [1.8999999999999986, 1.2, 98.61000000000001], center = false);
          }
          translate(v = [23, -3, 32.5])
          {
            cube(size = [1.8999999999999986, 1.2, 98.61000000000001], center = false);
          }
          translate(v = [23, 1.8, 32.5])
          {
            cube(size = [1.8999999999999986, 1.2, 98.61000000000001], center = false);
          }
        }
        translate(v = [0, 11.75, 130.10000000000002])
        {
          cylinder(h = 4.5, r = 4.5, center = false);
        }
        union()
        {
          translate(v = [-3.917691453623979, -20.356921938165303, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [0, -20.356921938165303, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [3.917691453623979, -20.356921938165303, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-9.794228634059948, -16.964101615137753, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-5.876537180435968, -16.964101615137753, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-1.9588457268119894, -16.964101615137753, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [1.9588457268119894, -16.964101615137753, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [5.876537180435968, -16.964101615137753, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [9.794228634059948, -16.964101615137753, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-15.670765814495915, -13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-11.753074360871937, -13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-7.835382907247958, -13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-3.917691453623979, -13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [0, -13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [3.917691453623979, -13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [7.835382907247958, -13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [11.753074360871937, -13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [15.670765814495915, -13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-17.629611541307906, -10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-13.711920087683925, -10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-9.794228634059948, -10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-5.876537180435968, -10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-1.9588457268119894, -10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [1.9588457268119894, -10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [5.876537180435968, -10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [9.794228634059948, -10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [13.711920087683925, -10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [17.629611541307906, -10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-19.588457268119896, -6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-15.670765814495915, -6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-11.753074360871937, -6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-7.835382907247958, -6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-3.917691453623979, -6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [0, -6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [3.917691453623979, -6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [7.835382907247958, -6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [11.753074360871937, -6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [15.670765814495915, -6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [19.588457268119896, -6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-19.588457268119896, 6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-15.670765814495915, 6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-11.753074360871937, 6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-7.835382907247958, 6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [7.835382907247958, 6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [11.753074360871937, 6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [15.670765814495915, 6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [19.588457268119896, 6.785640646055101, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-17.629611541307906, 10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-13.711920087683925, 10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-9.794228634059948, 10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [9.794228634059948, 10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [13.711920087683925, 10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [17.629611541307906, 10.178460969082652, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-15.670765814495915, 13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-11.753074360871937, 13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-7.835382907247958, 13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [7.835382907247958, 13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [11.753074360871937, 13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [15.670765814495915, 13.571281292110202, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-9.794228634059948, 16.964101615137753, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [9.794228634059948, 16.964101615137753, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [-3.917691453623979, 20.356921938165303, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [0, 20.356921938165303, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
          translate(v = [3.917691453623979, 20.356921938165303, 130.10000000000002])
          {
            rotate(a = [0, 0, 30], v = undef)
            {
              cylinder(h = 4.5, r = 1.8, center = false, $fn = 6);
            }
          }
        }
        union()
        {
          translate(v = [-27, 0, 26.5])
          {
            rotate(a = [0, 90, 0], v = undef)
            {
              cylinder(h = 54, r = 1.7, center = false);
            }
          }
          translate(v = [-26.01, 0, 26.5])
          {
            rotate(a = [0, 90, 0], v = undef)
            {
              cylinder(h = 1.3100000000000023, r1 = 3, r2 = 1.7, center = false);
            }
          }
        }
        union()
        {
          translate(v = [-27, 0, 26.5])
          {
            rotate(a = [0, 90, 0], v = undef)
            {
              cylinder(h = 54, r = 1.7, center = false);
            }
          }
          translate(v = [24.7, 0, 26.5])
          {
            rotate(a = [0, 90, 0], v = undef)
            {
              cylinder(h = 1.3100000000000023, r1 = 1.7, r2 = 3, center = false);
            }
          }
        }
      }
      translate(v = [0, -27, 23.5])
      {
        cube(size = [27, 54, 111.10000000000002], center = false);
      }
    }
  }
  color(c = "tomato", alpha = 1)
  {
    difference()
    {
      union()
      {
        translate(v = [-23.099999999999998, -1.5, 28.8])
        {
          cube(size = [46.199999999999996, 3, 4.709999999999997], center = false);
        }
        translate(v = [-23.9, -1.5, 33.5])
        {
          cube(size = [47.8, 3, 97.30000000000001], center = false);
        }
        union()
        {
          translate(v = [-5.6, 1.49, 35.8])
          {
            rotate(a = [-90, 0, 0], v = undef)
            {
              cylinder(h = 3.01, r = 2.65, center = false);
            }
          }
          translate(v = [5.6, 1.49, 35.8])
          {
            rotate(a = [-90, 0, 0], v = undef)
            {
              cylinder(h = 3.01, r = 2.65, center = false);
            }
          }
          translate(v = [-5.6, 1.49, 82.8])
          {
            rotate(a = [-90, 0, 0], v = undef)
            {
              cylinder(h = 3.01, r = 2.65, center = false);
            }
          }
          translate(v = [5.6, 1.49, 82.8])
          {
            rotate(a = [-90, 0, 0], v = undef)
            {
              cylinder(h = 3.01, r = 2.65, center = false);
            }
          }
        }
        union()
        {
          translate(v = [-11.5, 1.49, 91.30000000000001])
          {
            rotate(a = [-90, 0, 0], v = undef)
            {
              cylinder(h = 5.01, r = 3.05, center = false);
            }
          }
          translate(v = [11.5, 1.49, 91.30000000000001])
          {
            rotate(a = [-90, 0, 0], v = undef)
            {
              cylinder(h = 5.01, r = 3.05, center = false);
            }
          }
          translate(v = [-11.5, 1.49, 126.30000000000001])
          {
            rotate(a = [-90, 0, 0], v = undef)
            {
              cylinder(h = 5.01, r = 3.05, center = false);
            }
          }
          translate(v = [11.5, 1.49, 126.30000000000001])
          {
            rotate(a = [-90, 0, 0], v = undef)
            {
              cylinder(h = 5.01, r = 3.05, center = false);
            }
          }
        }
      }
      union()
      {
        translate(v = [-5.6, -2.5, 35.8])
        {
          rotate(a = [-90, 0, 0], v = undef)
          {
            cylinder(h = 8, r = 0.85, center = false);
          }
        }
        translate(v = [5.6, -2.5, 35.8])
        {
          rotate(a = [-90, 0, 0], v = undef)
          {
            cylinder(h = 8, r = 0.85, center = false);
          }
        }
        translate(v = [-5.6, -2.5, 82.8])
        {
          rotate(a = [-90, 0, 0], v = undef)
          {
            cylinder(h = 8, r = 0.85, center = false);
          }
        }
        translate(v = [5.6, -2.5, 82.8])
        {
          rotate(a = [-90, 0, 0], v = undef)
          {
            cylinder(h = 8, r = 0.85, center = false);
          }
        }
      }
      union()
      {
        translate(v = [-11.5, -2.5, 91.30000000000001])
        {
          rotate(a = [-90, 0, 0], v = undef)
          {
            cylinder(h = 10, r = 1.25, center = false);
          }
        }
        translate(v = [11.5, -2.5, 91.30000000000001])
        {
          rotate(a = [-90, 0, 0], v = undef)
          {
            cylinder(h = 10, r = 1.25, center = false);
          }
        }
        translate(v = [-11.5, -2.5, 126.30000000000001])
        {
          rotate(a = [-90, 0, 0], v = undef)
          {
            cylinder(h = 10, r = 1.25, center = false);
          }
        }
        translate(v = [11.5, -2.5, 126.30000000000001])
        {
          rotate(a = [-90, 0, 0], v = undef)
          {
            cylinder(h = 10, r = 1.25, center = false);
          }
        }
      }
      translate(v = [-17.75, -2.5, 41.3])
      {
        cube(size = [3, 5, 5], center = false);
      }
      translate(v = [14.75, -2.5, 41.3])
      {
        cube(size = [3, 5, 5], center = false);
      }
      translate(v = [-17.75, -2.5, 67.3])
      {
        cube(size = [3, 5, 5], center = false);
      }
      translate(v = [14.75, -2.5, 67.3])
      {
        cube(size = [3, 5, 5], center = false);
      }
      translate(v = [-24.9, -2.5, 34.5])
      {
        cube(size = [6, 5, 8], center = false);
      }
      translate(v = [18.9, -2.5, 34.5])
      {
        cube(size = [6, 5, 8], center = false);
      }
      translate(v = [-24.9, -2.5, 120.80000000000001])
      {
        cube(size = [6, 5, 8], center = false);
      }
      translate(v = [18.9, -2.5, 120.80000000000001])
      {
        cube(size = [6, 5, 8], center = false);
      }
    }
  }
  color(c = "green", alpha = 1)
  {
    translate(v = [-10.5, 4.5, 33.8])
    {
      cube(size = [21, 1, 51], center = false);
    }
  }
  color(c = "navy", alpha = 1)
  {
    translate(v = [-14, 6.5, 88.8])
    {
      cube(size = [28, 1.5999999999999996, 40.000000000000014], center = false);
    }
  }
  color(c = "deepskyblue", alpha = 1)
  {
    translate(v = [-5, 8.1, 121.30000000000001])
    {
      cube(size = [10, 10.000000000000002, 7.5], center = false);
    }
  }
  color(c = "dimgray", alpha = 1)
  {
    translate(v = [-13.75, -15.5, 33.8])
    {
      cube(size = [27.5, 14, 46], center = false);
    }
  }
}
