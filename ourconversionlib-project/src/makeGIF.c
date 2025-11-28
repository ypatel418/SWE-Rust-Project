/* MakeGIF.c using ImageMagick */
#include <stdio.h>
#include <stdlib.h>
#include <MagickWand/MagickWand.h>

static void print_wand_error(const char *where, MagickWand *w) 
{
    ExceptionType et;
    char *desc = MagickGetException(w, &et);
    if (desc) {
        fprintf(stderr, "[%s] Magick error (%d): %s\n", where, et, desc);
        MagickRelinquishMemory(desc);
    } else {
        fprintf(stderr, "[%s] (no exception text)\n", where);
    }
}


// GIF input structure (takes in frames, delay, loop, can accept width/height or default)
typedef struct GIFInput {
    const char **frames;
    size_t       count;
    const char  *out_gif;
    int          delay_cs;
    int          loop;
    size_t       target_w;
    size_t       target_h;
} GIFInput;

// Function to generate a GIFInput struct
GIFInput makeGIFInput(const char **frames, size_t count,
                      const char *out_gif,
                      int delay_cs, int loop,
                      size_t target_w, size_t target_h)
{
    GIFInput input;
    input.frames   = frames;
    input.count    = count;
    input.out_gif  = out_gif;
    input.delay_cs = delay_cs;
    input.loop     = loop;
    input.target_w = target_w;
    input.target_h = target_h;
    return input;
}

    // return code 1 -> set to 0 at end 
    int rc = 1;
    // ImageMagick version or True/ False used for if statements
    MagickBooleanType ok = MagickTrue;

int makeGIF(GIFInput input)
{
    if (!input.frames || input.count == 0 || !input.out_gif) {
        fprintf(stderr,
                "Error [makeGIF]: invalid arguments "
                "(frames=%p, count=%zu, out_gif=%p)\n",
                (void*)input.frames, input.count, (void*)input.out_gif);
        return 1;
    }



    // *anim =  MagickWand animation processes
    MagickWand *anim = NewMagickWand();
    if (!anim) {
        fprintf(stderr, "Error [makeGIF]: failed to allocate MagickWand\n");
        return 1;
    }

    size_t TW = input.target_w;
    size_t TH = input.target_h;

    // Auto size - use smallest frame dimensions if TW/TH are 0
    // This helps if images are of varying sizes, avoids blank space
    if (TW == 0 || TH == 0) {
        size_t minW = 0, minH = 0;
        MagickWand *probe = NewMagickWand();

        if (!probe) { //
            fprintf(stderr, "Error [makeGIF]: failed to allocate probe wand\n");
            goto done;
        }

        for (size_t i = 0; i < input.count; ++i) {
            if (!input.frames[i]) continue;

            // Make sure frames can be read
            // Compares to MagickTrue, if != returns error statement and breaks
            if (MagickReadImage(probe, input.frames[i]) != MagickTrue) {
                fprintf(stderr,
                        "Error [makeGIF]: failed to read frame %zu ('%s') for size probe\n",
                        i, input.frames[i]);
                print_wand_error("probe read", probe);
                ok = MagickFalse;
                break;
            }

            size_t w = MagickGetImageWidth(probe);
            size_t h = MagickGetImageHeight(probe);

            if (w == 0 || h == 0) {
                fprintf(stderr, "Error [makeGIF]: probe got zero size for frame %zu\n", i);
                ok = MagickFalse;
                break;
            }

            if (minW == 0 || w < minW) minW = w;
            if (minH == 0 || h < minH) minH = h;

            ClearMagickWand(probe);
        }

        DestroyMagickWand(probe);
        if (!ok) goto done;

        // This shouldn't ever happen, but just in case a user uploads something anomalous
        // that bypasses constraints set by frontend.
        if (minW == 0 || minH == 0) {
            fprintf(stderr, "Error [makeGIF]: could not determine minimum frame size\n");
            goto done;
        }

        if (TW == 0) TW = minW;
        if (TH == 0) TH = minH;
    }

    // Build frames     
    for (size_t i = 0; i < input.count; ++i) {
        const char *path = input.frames[i];
        if (!path) {
            fprintf(stderr, "Warning [makeGIF]: frame %zu path is NULL, skipping\n", i);
            continue;
        }

        MagickWand *w = NewMagickWand();
        if (!w) {
            fprintf(stderr, "Error [makeGIF]: failed to allocate wand for frame %zu\n", i);
            ok = MagickFalse;
            break;
        }

        if (MagickReadImage(w, path) != MagickTrue) {
            fprintf(stderr, "Error [makeGIF]: failed to read frame %zu ('%s')\n", i, path);
            print_wand_error("read", w);
            DestroyMagickWand(w);
            ok = MagickFalse;
            break;
        }

        // Original size
        size_t in_w = MagickGetImageWidth(w);
        size_t in_h = MagickGetImageHeight(w);
        if (in_w == 0 || in_h == 0) {
            fprintf(stderr, "Error [makeGIF]: frame %zu has zero size\n", i);
            DestroyMagickWand(w);
            ok = MagickFalse;
            break;
        }

        //SCALE TO FILL (cover), not fit.
        double sx = (double)TW / (double)in_w;
        double sy = (double)TH / (double)in_h;
        double s  = (sx > sy) ? sx : sy;  // MAX -> cover

        size_t rw = (size_t)(in_w * s + 0.5);
        size_t rh = (size_t)(in_h * s + 0.5);

        if (MagickResizeImage(w, rw, rh, LanczosFilter) != MagickTrue) {
            fprintf(stderr, "Error [makeGIF]: failed to resize frame %zu\n", i);
            print_wand_error("resize", w);
            DestroyMagickWand(w);
            ok = MagickFalse;
            break;
        }

        // center cropping to avoid blank spaces / unusuable gifs
        ssize_t cx = (ssize_t)((rw > TW) ? ((rw - TW) / 2) : 0);
        ssize_t cy = (ssize_t)((rh > TH) ? ((rh - TH) / 2) : 0);

        if (MagickCropImage(w, TW, TH, cx, cy) != MagickTrue) {
            fprintf(stderr, "Error [makeGIF]: failed to crop frame %zu\n", i);
            print_wand_error("crop", w);
            DestroyMagickWand(w);
            ok = MagickFalse;
            break;
        }

        // Ensure virtual canvas matches
        if (MagickSetImagePage(w, TW, TH, 0, 0) != MagickTrue) {
            fprintf(stderr, "Error [makeGIF]: failed to set page for frame %zu\n", i);
            print_wand_error("set page", w);
            DestroyMagickWand(w);
            ok = MagickFalse;
            break;
        }

        // Per-frame timing (delay is in centiseconds)
        if (MagickSetImageDelay(w, (size_t)input.delay_cs) != MagickTrue) {
            fprintf(stderr, "Error [makeGIF]: failed to set delay for frame %zu\n", i);
            print_wand_error("set delay", w);
            DestroyMagickWand(w);
            ok = MagickFalse;
            break;
        }

        // Disposal - clear to background before drawing next frame
        if (MagickSetImageDispose(w, BackgroundDispose) != MagickTrue) {
            fprintf(stderr, "Error [makeGIF]: failed to set dispose for frame %zu\n", i);
            print_wand_error("set dispose", w);
            DestroyMagickWand(w);
            ok = MagickFalse;
            break;
        }

        // Make sure it's GIF frame data
        if (MagickSetImageFormat(w, "GIF") != MagickTrue) {
            fprintf(stderr, "Error [makeGIF]: failed to set format GIF for frame %zu\n", i);
            print_wand_error("frame format", w);
            DestroyMagickWand(w);
            ok = MagickFalse;
            break;
        }

        if (MagickAddImage(anim, w) != MagickTrue) {
            fprintf(stderr, "Error [makeGIF]: failed to add frame %zu to animation\n", i);
            print_wand_error("add frame", anim);
            DestroyMagickWand(w);
            ok = MagickFalse;
            break;
        }

        DestroyMagickWand(w);
    }

    if (!ok) goto done;

    if (MagickGetNumberImages(anim) == 0) {
        fprintf(stderr, "Error [makeGIF]: no frames added to animation\n");
        goto done;
    }

    // Ensures iterator is at first frame before setting iterations
    MagickSetFirstIterator(anim);

    // Loop count (0 = infinite)
    if (MagickSetImageIterations(anim, (size_t)input.loop) != MagickTrue) {
        fprintf(stderr, "Error [makeGIF]: failed to set loop iterations (%d)\n", input.loop);
        print_wand_error("set iterations", anim);
        goto done;
    }

    // Basic optimization 
    (void)MagickOptimizeImageLayers(anim);
    // (void)MagickOptimizeImageTransparency(anim); 

    // Final write
    if (MagickWriteImages(anim, input.out_gif, MagickTrue) != MagickTrue) {
        fprintf(stderr, "Error [makeGIF]: failed to write GIF '%s'\n", input.out_gif);
        print_wand_error("write", anim);
        goto done;
    }

    // success message
    printf("Successfully created GIF '%s' with %zu frame(s), delay %d cs, loop=%d\n",
           input.out_gif, input.count, input.delay_cs, input.loop);
    
    // rc return code is 0 at end if everything has worked without error.
    rc = 0;

done:
    if (anim) {
        DestroyMagickWand(anim);
    }
    return rc;
}
