const ManifestPlugin = require('webpack-manifest-plugin');
const WorkboxWebpackPlugin = require('workbox-webpack-plugin');
const MiniCssExtractPlugin = require('mini-css-extract-plugin');
const CompressionPlugin = require('compression-webpack-plugin');
const ProgmemGenerator = require('./progmem-generator.js');

const path = require('path');
const fs = require('fs');

module.exports = function override(config, env) {
  if (env === "production") {
    // rename the ouput file, we need it's path to be short, for embedded FS
    config.output.filename = 'js/[id].[hash:4].js';
    config.output.chunkFilename = 'js/[id].[hash:4].js';

    // take out the manifest and service worker plugins
    config.plugins = config.plugins.filter(plugin => !(plugin instanceof ManifestPlugin));
    config.plugins = config.plugins.filter(plugin => !(plugin instanceof WorkboxWebpackPlugin.GenerateSW));

    // shorten css filenames
    const miniCssExtractPlugin = config.plugins.find((plugin) => plugin instanceof MiniCssExtractPlugin);
    miniCssExtractPlugin.options.filename = "css/[id].[hash:4].css";
    miniCssExtractPlugin.options.chunkFilename = "css/[id].[hash:4].c.css";

    // build progmem data files
    config.plugins.push(new ProgmemGenerator({ outputPath: "../WWWData.h", bytesPerLine: 20 }));

    // add compression plugin, compress javascript
    config.plugins.push(new CompressionPlugin({
      filename: "[path].gz[query]",
      algorithm: "gzip",
      test: /\.(js)$/,
      deleteOriginalAssets: true
    }));
    config.optimization.splitChunks = {
         chunks: 'all',
      }
  }
  return config;
}
