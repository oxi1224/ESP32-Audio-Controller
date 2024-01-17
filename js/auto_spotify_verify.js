import open from 'open';

(async () => {
  await new Promise(r => setTimeout(r, 500));
  const url = "https://accounts.spotify.com/authorize?client_id=0636912a16074ecabec2491683dad579&response_type=code&redirect_uri=http://192.168.50.143:4001/callback&scope=user-read-playback-state%20user-read-currently-playing";
  open(url);
})(); 