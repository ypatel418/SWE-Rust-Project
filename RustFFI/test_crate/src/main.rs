use image_conversion_wrapper::*;

fn main() {
    // Input and Quality
    convert_to_jpg("input.png", 100);
    convert_to_png("input.png", 100);
    convert_to_tiff("input.png", 100);
    convert_to_webp("input.png", 100);

    //Format for GIF: frames, delay, loop count, and width/height.
    let gif_input = make_gif_input(&["frame1.jpg", "frame2.jpg","frame3.jpg"], "output.gif", 100, 3, 0, 0);
    
    make_gif(gif_input);

}