const memoryCache = require('memory-cache');

const ITS_BEEN__ONE_WEEK_SINCE_YOU_LOOKED_AT_ME = 1000 * 60 * 60 * 24 * 7;

module.exports = function cacheMiddleware(req, res, next) {
  const key = req.originalUrl || req.url;
  const cached = memoryCache.get(key);
  if (cached) return res.send(cached);
  res.sendResponse = res.send = (contents) => {
    memoryCache.put(key, contents, ITS_BEEN__ONE_WEEK_SINCE_YOU_LOOKED_AT_ME);
  };
  next();
};
