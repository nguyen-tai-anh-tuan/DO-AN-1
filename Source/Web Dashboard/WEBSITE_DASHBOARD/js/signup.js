document.addEventListener('DOMContentLoaded', () => {
    const form = document.querySelector('#signup form');
    if (form) {
        form.addEventListener('submit', (e) => {
            e.preventDefault();
            const fullname = document.getElementById('fullname').value;
            const email = document.getElementById('email').value;
            const password = document.getElementById('password').value;
            const confirmPassword = document.getElementById('confirm-password').value;

            if (!fullname || !email || !password || !confirmPassword) {
                alert('Vui lòng điền đầy đủ thông tin!');
                return;
            }
            if (!email.includes('@')) {
                alert('Email không hợp lệ!');
                return;
            }
            if (password.length < 6) {
                alert('Mật khẩu phải có ít nhất 6 ký tự!');
                return;
            }
            if (password !== confirmPassword) {
                alert('Mật khẩu xác nhận không khớp!');
                return;
            }
            alert('Đăng ký thành công! (Chức năng demo)');
            form.reset();
        });
    }
});