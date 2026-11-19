// Written by tools/render_faceplate.ts (mise run faceplate): don't edit. Preview (F5) keeps the colors.
$fn = 64;
union()
{
  union()
  {
    color(c = "gold", alpha = 1)
    {
      union()
      {
        difference()
        {
          translate(v = [0, -8.768988277011857, 0])
          {
            linear_extrude(height = 17.9, center = false, convexity = undef, twist = undef, slices = undef, scale = 1, $fn = 20)
            {
              offset(r = 3, delta = undef, chamfer = undef)
              {
                square(size = [157.79999999999998, 52.83797655402372], center = true);
              }
            }
          }
          translate(v = [0, -220.65, -1])
          {
            cylinder(h = 19.9, r = 200, center = false, $fn = 720);
          }
          translate(v = [-80.3, -19.05, 5.4])
          {
            cube(size = [111.1, 38.1, 13.499999999999998], center = false);
          }
          translate(v = [30.789999999999996, -19.05, 1.6])
          {
            cube(size = [49.510000000000005, 38.1, 17.299999999999997], center = false);
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
          translate(v = [-142.89999999999998, 0, 0])
          {
            rotate(a = [0, 0, 90], v = undef)
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
          }
          union()
          {
            translate(v = [-57.599999999999994, -39.18797655402372, 9.099999999999998])
            {
              cube(size = [10.599999999999994, 20.14797655402372, 1.6500000000000004], center = false);
            }
            translate(v = [-57.599999999999994, 19.04, 9.099999999999998])
            {
              cube(size = [10.599999999999994, 20.14797655402372, 1.6500000000000004], center = false);
            }
            translate(v = [-5.3, -39.18797655402372, 9.099999999999998])
            {
              cube(size = [10.6, 20.14797655402372, 1.6500000000000004], center = false);
            }
            translate(v = [-5.3, 19.04, 9.099999999999998])
            {
              cube(size = [10.6, 20.14797655402372, 1.6500000000000004], center = false);
            }
            translate(v = [47, -39.18797655402372, 9.099999999999998])
            {
              cube(size = [10.599999999999994, 20.14797655402372, 1.6500000000000004], center = false);
            }
            translate(v = [47, 19.04, 9.099999999999998])
            {
              cube(size = [10.599999999999994, 20.14797655402372, 1.6500000000000004], center = false);
            }
          }
        }
        union()
        {
          translate(v = [-26.5, -15.25, 4.19])
          {
            cylinder(h = 2.71, r = 1.2, center = false);
          }
          translate(v = [-26.5, 15.25, 4.19])
          {
            cylinder(h = 2.71, r = 1.2, center = false);
          }
          translate(v = [26.5, -15.25, 4.19])
          {
            cylinder(h = 2.71, r = 1.2, center = false);
          }
          translate(v = [26.5, 15.25, 4.19])
          {
            cylinder(h = 2.71, r = 1.2, center = false);
          }
        }
        difference()
        {
          translate(v = [-142.89999999999998, 0, 0])
          {
            rotate(a = [0, 0, 90], v = undef)
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
          translate(v = [-142.89999999999998, 0, 0])
          {
            rotate(a = [0, 0, 90], v = undef)
            {
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
        }
        union()
        {
          union()
          {
            translate(v = [34.8, -11.5, 1.59])
            {
              cylinder(h = 3.01, r = 3, center = false);
            }
            translate(v = [34.8, 11.5, 1.59])
            {
              cylinder(h = 3.01, r = 3, center = false);
            }
            translate(v = [69.8, -11.5, 1.59])
            {
              cylinder(h = 3.01, r = 3, center = false);
            }
            translate(v = [69.8, 11.5, 1.59])
            {
              cylinder(h = 3.01, r = 3, center = false);
            }
          }
          union()
          {
            translate(v = [34.8, -11.5, 4.59])
            {
              cylinder(h = 3.1099999999999994, r = 1.45, center = false);
            }
            translate(v = [34.8, 11.5, 4.59])
            {
              cylinder(h = 3.1099999999999994, r = 1.45, center = false);
            }
            translate(v = [69.8, -11.5, 4.59])
            {
              cylinder(h = 3.1099999999999994, r = 1.45, center = false);
            }
            translate(v = [69.8, 11.5, 4.59])
            {
              cylinder(h = 3.1099999999999994, r = 1.45, center = false);
            }
          }
        }
      }
    }
    union()
    {
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
    }
    intersection()
    {
      translate(v = [-142.89999999999998, 0, 0])
      {
        rotate(a = [0, 0, 90], v = undef)
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
      }
      translate(v = [-500, -19.05, -50])
      {
        cube(size = [1000, 38.1, 100], center = false);
      }
    }
    color(c = "black", alpha = 1)
    {
      translate(v = [-47, -5.5, 6.1000000000000005])
      {
        cube(size = [18, 11, 7.000000000000001], center = false);
      }
    }
    union()
    {
      color(c = "navy", alpha = 1)
      {
        translate(v = [32.3, -14, 4.6])
        {
          cube(size = [40, 28, 1.5999999999999996], center = false);
        }
      }
      color(c = "deepskyblue", alpha = 1)
      {
        translate(v = [64.8, -5, 6.199999999999999])
        {
          cube(size = [7.5, 10, 10], center = false);
        }
      }
      color(c = "black", alpha = 1)
      {
        translate(v = [33.8, -8.9, 6.199999999999999])
        {
          cube(size = [2.5, 17.8, 2.5], center = false);
        }
      }
      color(c = "black", alpha = 1)
      {
        translate(v = [50.3, -4, 6.199999999999999])
        {
          cube(size = [8, 8, 1.5], center = false);
        }
      }
      color(c = "silver", alpha = 1)
      {
        translate(v = [49.3, 6, 6.199999999999999])
        {
          cube(size = [11, 4.5, 3.5], center = false);
        }
      }
    }
  }
  color(c = "silver", alpha = 1)
  {
    difference()
    {
      union()
      {
        difference()
        {
          translate(v = [0, -8.768988277011857, 17.9])
          {
            linear_extrude(height = 1.1999999999999993, center = false, convexity = undef, twist = undef, slices = undef, scale = 1, $fn = 20)
            {
              offset(r = 3, delta = undef, chamfer = undef)
              {
                square(size = [157.79999999999998, 52.83797655402372], center = true);
              }
            }
          }
          translate(v = [0, -220.65, 16.9])
          {
            cylinder(h = 3.1999999999999993, r = 200, center = false, $fn = 720);
          }
        }
        union()
        {
          union()
          {
            translate(v = [-57.3, -18.75, 9.399999999999999])
            {
              cube(size = [10, 1, 8.510000000000002], center = false);
            }
            hull()
            {
              translate(v = [-57.3, -18.75, 9.399999999999999])
              {
                cube(size = [10, 0.010000000000001563, 0.009999999999999787], center = false);
              }
              translate(v = [-57.3, -20.05, 10.589999999999998])
              {
                cube(size = [10, 1.3100000000000023, 0.009999999999999787], center = false);
              }
            }
          }
          union()
          {
            translate(v = [-57.3, 17.75, 9.399999999999999])
            {
              cube(size = [10, 1, 8.510000000000002], center = false);
            }
            hull()
            {
              translate(v = [-57.3, 18.74, 9.399999999999999])
              {
                cube(size = [10, 0.010000000000001563, 0.009999999999999787], center = false);
              }
              translate(v = [-57.3, 18.74, 10.589999999999998])
              {
                cube(size = [10, 1.3100000000000023, 0.009999999999999787], center = false);
              }
            }
          }
          union()
          {
            translate(v = [-5, -18.75, 9.399999999999999])
            {
              cube(size = [10, 1, 8.510000000000002], center = false);
            }
            hull()
            {
              translate(v = [-5, -18.75, 9.399999999999999])
              {
                cube(size = [10, 0.010000000000001563, 0.009999999999999787], center = false);
              }
              translate(v = [-5, -20.05, 10.589999999999998])
              {
                cube(size = [10, 1.3100000000000023, 0.009999999999999787], center = false);
              }
            }
          }
          union()
          {
            translate(v = [-5, 17.75, 9.399999999999999])
            {
              cube(size = [10, 1, 8.510000000000002], center = false);
            }
            hull()
            {
              translate(v = [-5, 18.74, 9.399999999999999])
              {
                cube(size = [10, 0.010000000000001563, 0.009999999999999787], center = false);
              }
              translate(v = [-5, 18.74, 10.589999999999998])
              {
                cube(size = [10, 1.3100000000000023, 0.009999999999999787], center = false);
              }
            }
          }
          union()
          {
            translate(v = [47.3, -18.75, 9.399999999999999])
            {
              cube(size = [10, 1, 8.510000000000002], center = false);
            }
            hull()
            {
              translate(v = [47.3, -18.75, 9.399999999999999])
              {
                cube(size = [10, 0.010000000000001563, 0.009999999999999787], center = false);
              }
              translate(v = [47.3, -20.05, 10.589999999999998])
              {
                cube(size = [10, 1.3100000000000023, 0.009999999999999787], center = false);
              }
            }
          }
          union()
          {
            translate(v = [47.3, 17.75, 9.399999999999999])
            {
              cube(size = [10, 1, 8.510000000000002], center = false);
            }
            hull()
            {
              translate(v = [47.3, 18.74, 9.399999999999999])
              {
                cube(size = [10, 0.010000000000001563, 0.009999999999999787], center = false);
              }
              translate(v = [47.3, 18.74, 10.589999999999998])
              {
                cube(size = [10, 1.3100000000000023, 0.009999999999999787], center = false);
              }
            }
          }
        }
        union()
        {
          difference()
          {
            union()
            {
              translate(v = [-25.7, -5.6, 11.899999999999999])
              {
                cylinder(h = 6.010000000000002, r = 1.7, center = false);
              }
            }
            translate(v = [-30.7, -4.3, 10.899999999999999])
            {
              cube(size = [10, 8.6, 8], center = false);
            }
          }
          difference()
          {
            union()
            {
              translate(v = [-25.7, 5.6, 11.899999999999999])
              {
                cylinder(h = 6.010000000000002, r = 1.7, center = false);
              }
            }
            translate(v = [-30.7, -4.3, 10.899999999999999])
            {
              cube(size = [10, 8.6, 8], center = false);
            }
          }
          difference()
          {
            union()
            {
              translate(v = [21.3, -5.6, 11.899999999999999])
              {
                cylinder(h = 6.010000000000002, r = 1.7, center = false);
              }
            }
            translate(v = [16.3, -4.3, 10.899999999999999])
            {
              cube(size = [10, 8.6, 8], center = false);
            }
          }
          difference()
          {
            union()
            {
              translate(v = [21.3, 5.6, 11.899999999999999])
              {
                cylinder(h = 6.010000000000002, r = 1.7, center = false);
              }
            }
            translate(v = [16.3, -4.3, 10.899999999999999])
            {
              cube(size = [10, 8.6, 8], center = false);
            }
          }
          union()
          {
            translate(v = [-25.7, -5.6, 10.3])
            {
              cylinder(h = 1.6099999999999977, r = 0.8, center = false);
            }
          }
          union()
          {
            translate(v = [-25.7, 5.6, 10.3])
            {
              cylinder(h = 1.6099999999999977, r = 0.8, center = false);
            }
          }
          union()
          {
            translate(v = [21.3, -5.6, 9.4])
            {
              cylinder(h = 2.509999999999998, r = 0.8, center = false);
            }
          }
          union()
          {
            translate(v = [21.3, 5.6, 9.4])
            {
              cylinder(h = 2.509999999999998, r = 0.8, center = false);
            }
          }
        }
      }
      hull()
      {
        translate(v = [76.3, -4.5, 16.9])
        {
          cylinder(h = 3.2, r = 3.5, center = false);
        }
        translate(v = [76.3, 4.5, 16.9])
        {
          cylinder(h = 3.2, r = 3.5, center = false);
        }
      }
      hull()
      {
        translate(v = [-51.49999999999999, 0, 16.9])
        {
          cylinder(h = 3.2, r = 6, center = false);
        }
        translate(v = [-35.3, 0, 16.9])
        {
          cylinder(h = 3.2, r = 6, center = false);
        }
      }
      union()
      {
        translate(v = [-74.4361376188556, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-70.51844616523162, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-66.60075471160764, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-62.68306325798366, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-58.76537180435968, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-54.8476803507357, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-50.92998889711173, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-47.012297443487746, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-43.094605989863766, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-39.17691453623979, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-35.25922308261581, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-31.34153162899183, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-27.42384017536785, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-23.506148721743873, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-19.588457268119896, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-15.670765814495915, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-11.753074360871937, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-7.835382907247958, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-3.917691453623979, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [0, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [3.917691453623979, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [7.835382907247958, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [11.753074360871937, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [15.670765814495915, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [19.588457268119896, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [23.506148721743873, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [27.42384017536785, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [31.34153162899183, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [35.25922308261581, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [39.17691453623979, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [43.094605989863766, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [47.012297443487746, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [50.92998889711173, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [54.8476803507357, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [58.76537180435968, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [62.68306325798366, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [66.60075471160764, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [70.51844616523162, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [74.4361376188556, -13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-76.3949833456676, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-72.47729189204361, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-68.55960043841964, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-64.64190898479565, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-60.724217531171675, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-56.806526077547694, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-52.888834623923714, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-48.97114317029974, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-45.05345171667576, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-41.13576026305178, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-37.218068809427805, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-33.300377355803825, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-29.38268590217984, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-25.46499444855586, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-21.547302994931883, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-17.629611541307906, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-13.711920087683925, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-9.794228634059948, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-5.876537180435968, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-1.9588457268119894, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [1.9588457268119894, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [5.876537180435968, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [9.794228634059948, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [13.711920087683925, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [17.629611541307906, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [21.547302994931886, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [25.464994448555863, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [29.38268590217984, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [33.30037735580382, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [37.2180688094278, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [41.13576026305178, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [45.05345171667575, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [48.97114317029973, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [52.888834623923714, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [56.80652607754769, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [60.72421753117167, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [64.64190898479565, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [68.55960043841962, -10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-74.4361376188556, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-70.51844616523162, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-66.60075471160764, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-62.68306325798366, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-19.588457268119896, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-15.670765814495915, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-11.753074360871937, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-7.835382907247958, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-3.917691453623979, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [0, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [3.917691453623979, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [7.835382907247958, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [11.753074360871937, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [15.670765814495915, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [27.42384017536785, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [31.34153162899183, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [35.25922308261581, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [39.17691453623979, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [43.094605989863766, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [47.012297443487746, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [50.92998889711173, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [54.8476803507357, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [58.76537180435968, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [62.68306325798366, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [66.60075471160764, -6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-76.3949833456676, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-72.47729189204361, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-68.55960043841964, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-64.64190898479565, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-60.724217531171675, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-17.629611541307906, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-13.711920087683925, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-9.794228634059948, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-5.876537180435968, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-1.9588457268119894, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [1.9588457268119894, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [5.876537180435968, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [9.794228634059948, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [13.711920087683925, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [29.38268590217984, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [33.30037735580382, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [37.2180688094278, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [41.13576026305178, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [45.05345171667575, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [48.97114317029973, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [52.888834623923714, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [56.80652607754769, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [60.72421753117167, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [64.64190898479565, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [68.55960043841962, -3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-74.4361376188556, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-70.51844616523162, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-66.60075471160764, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-62.68306325798366, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-23.506148721743873, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-19.588457268119896, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-15.670765814495915, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-11.753074360871937, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-7.835382907247958, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-3.917691453623979, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [3.917691453623979, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [7.835382907247958, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [11.753074360871937, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [15.670765814495915, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [19.588457268119896, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [23.506148721743873, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [27.42384017536785, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [31.34153162899183, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [35.25922308261581, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [39.17691453623979, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [43.094605989863766, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [47.012297443487746, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [50.92998889711173, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [54.8476803507357, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [58.76537180435968, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [62.68306325798366, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [66.60075471160764, 0, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-76.3949833456676, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-72.47729189204361, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-68.55960043841964, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-64.64190898479565, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-60.724217531171675, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-17.629611541307906, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-13.711920087683925, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-9.794228634059948, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-5.876537180435968, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-1.9588457268119894, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [1.9588457268119894, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [5.876537180435968, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [9.794228634059948, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [13.711920087683925, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [29.38268590217984, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [33.30037735580382, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [37.2180688094278, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [41.13576026305178, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [45.05345171667575, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [48.97114317029973, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [52.888834623923714, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [56.80652607754769, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [60.72421753117167, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [64.64190898479565, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [68.55960043841962, 3.3928203230275504, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-74.4361376188556, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-70.51844616523162, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-66.60075471160764, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-62.68306325798366, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-19.588457268119896, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-15.670765814495915, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-11.753074360871937, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-7.835382907247958, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-3.917691453623979, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [0, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [3.917691453623979, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [7.835382907247958, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [11.753074360871937, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [15.670765814495915, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [27.42384017536785, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [31.34153162899183, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [35.25922308261581, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [39.17691453623979, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [43.094605989863766, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [47.012297443487746, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [50.92998889711173, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [54.8476803507357, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [58.76537180435968, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [62.68306325798366, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [66.60075471160764, 6.785640646055101, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-76.3949833456676, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-72.47729189204361, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-68.55960043841964, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-64.64190898479565, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-60.724217531171675, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-56.806526077547694, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-52.888834623923714, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-48.97114317029974, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-45.05345171667576, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-41.13576026305178, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-37.218068809427805, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-33.300377355803825, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-29.38268590217984, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-25.46499444855586, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-21.547302994931883, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-17.629611541307906, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-13.711920087683925, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-9.794228634059948, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-5.876537180435968, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-1.9588457268119894, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [1.9588457268119894, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [5.876537180435968, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [9.794228634059948, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [13.711920087683925, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [17.629611541307906, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [21.547302994931886, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [25.464994448555863, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [29.38268590217984, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [33.30037735580382, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [37.2180688094278, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [41.13576026305178, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [45.05345171667575, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [48.97114317029973, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [52.888834623923714, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [56.80652607754769, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [60.72421753117167, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [64.64190898479565, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [68.55960043841962, 10.178460969082652, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-74.4361376188556, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-70.51844616523162, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-66.60075471160764, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-62.68306325798366, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-58.76537180435968, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-54.8476803507357, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-50.92998889711173, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-47.012297443487746, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-43.094605989863766, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-39.17691453623979, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-35.25922308261581, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-31.34153162899183, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-27.42384017536785, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-23.506148721743873, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-19.588457268119896, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-15.670765814495915, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-11.753074360871937, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-7.835382907247958, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [-3.917691453623979, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [0, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [3.917691453623979, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [7.835382907247958, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [11.753074360871937, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [15.670765814495915, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [19.588457268119896, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [23.506148721743873, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [27.42384017536785, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [31.34153162899183, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [35.25922308261581, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [39.17691453623979, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [43.094605989863766, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [47.012297443487746, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [50.92998889711173, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [54.8476803507357, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [58.76537180435968, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [62.68306325798366, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [66.60075471160764, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [70.51844616523162, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
        translate(v = [74.4361376188556, 13.571281292110202, 16.9])
        {
          rotate(a = [0, 0, 30], v = undef)
          {
            cylinder(h = 3.1999999999999993, r = 1.8, center = false, $fn = 6);
          }
        }
      }
    }
  }
}
