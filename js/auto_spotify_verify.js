import open from 'open';

(async () => {
  await new Promise(r => setTimeout(r, 500));
  const url = "https://accounts.spotify.com/authorize?client_id=CLIENT_ID&response_type=code&redirect_uri=REDIRECT_URI&scope=user-read-playback-state%20user-read-currently-playing";
  open(url);
})(); 