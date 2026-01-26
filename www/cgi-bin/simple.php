#!/usr/bin/php
<?php
// Simple CGI output
echo "Content-Type: text/html\r\n";
echo "\r\n";  // Empty line separator
?>
<!DOCTYPE html>
<html>
<head><title>PHP Test 1</title></head>
<body>
    <h1>Basic PHP CGI Test</h1>
    <p>Server Time: <?php echo date('Y-m-d H:i:s'); ?></p>
    <p>PHP Version: <?php echo phpversion(); ?></p>
</body>
</html>