document.addEventListener('DOMContentLoaded', () => {
    // Fade-in animation on scroll
    const elements = document.querySelectorAll('.fade-in');
    const timelineItems = document.querySelectorAll('.timeline-item');

    const observerOptions = {
        threshold: 0.1,
        rootMargin: "0px 0px 100px 0px" // Thêm margin để kích hoạt sớm hơn khi cuộn
    };

    const observer = new IntersectionObserver((entries, observer) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                entry.target.classList.add('visible');
                observer.unobserve(entry.target); // Ngừng theo dõi sau khi hiển thị
            }
        });
    }, observerOptions);

    elements.forEach(element => observer.observe(element));
    timelineItems.forEach(item => observer.observe(item));

    // Initialize EmailJS
    emailjs.init('YOUR_USER_ID'); // Thay YOUR_USER_ID bằng User ID của bạn

    // Contact Form Submission
    const contactForm = document.getElementById('contact-form');
    if (contactForm) {
        contactForm.addEventListener('submit', (e) => {
            e.preventDefault();
            const name = document.getElementById('contact-name').value;
            const email = document.getElementById('contact-email').value;
            const message = document.getElementById('contact-message').value;
            const location = document.getElementById('contact-location').value;

            if (!name || !email || !message) {
                alert('Vui lòng điền đầy đủ thông tin!');
                return;
            }
            if (!email.includes('@')) {
                alert('Email không hợp lệ!');
                return;
            }

            // Gửi email qua EmailJS
            const emailParams = {
                from_name: name,
                from_email: email,
                message: message,
                location: location || 'Không có thông tin địa điểm'
            };

            emailjs.send('YOUR_SERVICE_ID', 'YOUR_TEMPLATE_ID', emailParams)
                .then(() => {
                    alert('Tin nhắn đã được gửi qua email thành công!');
                })
                .catch((error) => {
                    console.error('Gửi email thất bại:', error);
                    alert('Có lỗi xảy ra khi gửi email, vui lòng thử lại!');
                });

            // Gửi dữ liệu qua Google Sheets
            sendToGoogleSheets(name, email, message, location);

            // Reset form
            contactForm.reset();
        });
    }
});

// Hàm gửi dữ liệu đến Google Sheets
function sendToGoogleSheets(name, email, message, location) {
    const scriptURL = 'YOUR_GOOGLE_SCRIPT_URL'; // Thay bằng URL script Google Apps Script
    const data = {
        name: name,
        email: email,
        message: message,
        location: location || 'Không có thông tin địa điểm',
        timestamp: new Date().toISOString()
    };

    fetch(scriptURL, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
    })
    .then(response => {
        if (response.ok) {
            alert('Dữ liệu đã được gửi đến Google Sheets thành công!');
        } else {
            throw new Error('Gửi dữ liệu đến Google Sheets thất bại');
        }
    })
    .catch(error => {
        console.error('Lỗi:', error);
        alert('Có lỗi xảy ra khi gửi dữ liệu đến Google Sheets!');
    });
}