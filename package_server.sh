cp ./ai-arena.server.config.json ./ai-arena.config.json
zip bomberman-server server/* common/* CMakeLists.txt ai-arena.config.json
rm ai-arena.config.json
mongo ai-arena --eval "db.Game.update({name: \"Bomberman\"}, {\$set: {\"server.file\": new BinData(0, \"$(base64 bomberman-server.zip -w 0)\")}})"
