document.addEventListener('DOMContentLoaded', () => {
    const form = document.querySelector('#login form');
    if (form) {
        form.addEventListener('submit', (e) => {
            e.preventDefault();
            const email = document.getElementById('email').value;
            const password = document.getElementById('password').value;

            if (!email || !password) {
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
            alert('Đăng nhập thành công! (Chức năng demo)');
            form.reset();
        });
    }
});