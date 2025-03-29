const express = require('express');
const net = require('net');
const cors = require('cors');
const path = require('path');

const app = express();
app.use(cors());
app.use(express.json());
app.use(express.static('public'));

const SERVER_HOST = '127.0.0.1';
const SERVER_PORT = 27000;

function sendToServer(requestString) {
  return new Promise((resolve, reject) => {
    const client = new net.Socket();
    let response = '';

    client.connect(SERVER_PORT, SERVER_HOST, () => {
      client.write(requestString);
    });

    client.on('data', (data) => {
      response += data.toString();
    });

    client.on('end', () => {
      resolve(response.trim());
    });

    client.on('error', (err) => {
      reject(err);
    });
  });
}

// USER LOGIN
app.post('/login', async (req, res) => {
  const { username, password } = req.body;
  const request = `POST/USER/${username}/${password}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// DEVICE ADD
app.post('/lights', async (req, res) => {
  const { username, location, ip } = req.body;
  const request = `POST/L/AL/${username}/${location}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.post('/thermostats', async (req, res) => {
  const { username, location, ip } = req.body;
  const request = `POST/T/AT/${username}/${location}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.post('/cameras', async (req, res) => {
  const { username, location, ip } = req.body;
  const request = `POST/C/AC/${username}/${location}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// DEVICE DELETE
app.delete('/lights/:ip', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.body;
  const request = `DELETE/L/DD/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.delete('/thermostats/:ip', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.body;
  const request = `DELETE/T/DD/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.delete('/cameras/:ip', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.body;
  const request = `DELETE/C/DD/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// CONTROL ROUTES — LIGHTS
app.put('/lights/:ip/on', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.body;
  const request = `PUT/L/ON/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.put('/lights/:ip/off', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.body;
  const request = `PUT/L/OF/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.put('/lights/:ip/bulb/replace', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.body;
  const request = `PUT/L/RB/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// CONTROL ROUTES — THERMOSTATS
app.put('/thermostats/:ip/on', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.body;
  const request = `PUT/T/ON/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.put('/thermostats/:ip/off', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.body;
  const request = `PUT/T/OF/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.put('/thermostats/:ip/set', async (req, res) => {
  const { ip } = req.params;
  const { username, temperature } = req.body;
  const request = `PUT/T/ST/${username}/${ip}/${temperature}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// CONTROL ROUTES — CAMERAS
app.put('/cameras/:ip/on', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.body;
  const request = `PUT/C/ON/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.put('/cameras/:ip/off', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.body;
  const request = `PUT/C/OF/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.put('/cameras/:ip/memory/wipe', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.body;
  const request = `PUT/C/WM/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/lights/:ip/status/on', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.query;
  const request = `GET/L/ON/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/lights/:ip/status/bulb', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.query;
  const request = `GET/L/BO/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/lights/:ip/status', async (req, res) => {
    const { ip } = req.params;
    const { username } = req.query;
    const request = `GET/L/ST/${username}/${ip}`;
    try {
      const result = await sendToServer(request);
      res.json({ message: result });
    } catch (err) {
      res.status(500).json({ error: err.message });
    }
  });

app.get('/thermostats/:ip/temp/current', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.query;
  const request = `GET/T/CT/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/thermostats/:ip/temp/desired', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.query;
  const request = `GET/T/DT/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/thermostats/:ip/status', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.query;
  const request = `GET/T/ST/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/cameras/:ip/status/motion', async (req, res) => {
  const { ip } = req.params;
  const { username } = req.query;
  const request = `GET/C/MA/${username}/${ip}`;
  try {
    const result = await sendToServer(request);
    res.json({ message: result });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

app.get('/cameras/:ip/status', async (req, res) => {
    const { ip } = req.params;
    const { username } = req.query;
    const request = `GET/C/ST/${username}/${ip}`;
    try {
      const result = await sendToServer(request);
      res.json({ message: result });
    } catch (err) {
      res.status(500).json({ error: err.message });
    }
  });
app.listen(3000, () => {
  console.log('Server is running at http://localhost:3000');
});
