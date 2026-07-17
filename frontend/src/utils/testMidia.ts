const VIDEO_EXTENSIONS = /\.(mp4|mkv|avi|mov|wmv|flv|webm|m4v|mpg|mpeg|3gp)$/i;

export const isVideo = (src: string) => {
  return VIDEO_EXTENSIONS.test(src);
};
